// Copyright (c) 2018, ZIH, Technische Universitaet Dresden, Federal Republic of Germany
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of metricq nor the names of its contributors
//       may be used to endorse or promote products derived from this software
//       without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "connection_handler.hpp"
#include "log.hpp"

namespace metricq
{

void QueuedBuffer::emplace(const char* ptr, std::size_t size)
{
    // check if this fits at the back of the last queued buffer
    if (!empty() && buffers_.back().size() + size <= buffers_.back().capacity())
    {
        log::debug("Reusing buffer of size {} for {} bytes", buffers_.back().size(), size);
        std::copy(ptr, ptr + size, std::back_inserter(buffers_.back()));
    }
    else
    {

        if (empty_buffers_.empty()  || empty_buffers_.front().capacity() < size)
        {
            log::debug("Allocating buffer for {} bytes", size);

            buffers_.emplace();
            buffers_.back().reserve(std::max<std::size_t>(size, 4096));
        }
        else
        {
            log::debug("Reusing buffer from cached buffers for {} bytes", size);

            buffers_.emplace(std::move(empty_buffers_.front()));
            empty_buffers_.pop();
        }

        std::copy(ptr, ptr + size, std::back_inserter(buffers_.back()));
    }
}

void QueuedBuffer::consume(std::size_t consumed_bytes)
{
    assert(!empty());
    //        assert(buffers_.front().size() <= offset_ + consumed_bytes);

    if (buffers_.front().size() == offset_ + consumed_bytes)
    {
        offset_ = 0;
        buffers_.front().resize(0);
        empty_buffers_.emplace(std::move(buffers_.front()));
        buffers_.pop();
    }
    else
    {
        offset_ += consumed_bytes;
    }
}

ConnectionHandler::ConnectionHandler(asio::io_service& io_service)
: reconnect_timer_(io_service), heartbeat_timer_(io_service), resolver_(io_service),
  socket_(io_service)
{
}

void ConnectionHandler::connect(const AMQP::Address& address)
{
    assert(!socket_.is_open());
    assert(!connection_);

    address_ = address;
    connection_ = std::make_unique<AMQP::Connection>(this, address.login(), address.vhost());

    asio::ip::tcp::resolver::query query(address.hostname(), std::to_string(address.port()));
    resolver_.async_resolve(query, [this, address](const auto& error, auto endpoint_iterator) {
        if (error)
        {
            log::error("failed to resolve hostname {}: {}", address.hostname(), error.message());
            this->onError("resolve failed");
            return;
        }

        for (auto it = endpoint_iterator; it != decltype(endpoint_iterator)(); ++it)
        {
            log::debug("resolved {} to {}", address.hostname(), it->endpoint());
        }

        this->connect(endpoint_iterator);
    });
}

void ConnectionHandler::connect(asio::ip::tcp::resolver::iterator endpoint_iterator)
{
    asio::async_connect(
        this->socket_, endpoint_iterator, [this](const auto& error, auto successful_endpoint) {
            if (error)
            {
                log::error("connection failed: {}", error.message());
                this->onError("connect failed");
                return;
            }
            log::debug("successfully connected to {} at {}", successful_endpoint->host_name(),
                       successful_endpoint->endpoint());
            this->read();
            this->flush();
        });
}

uint16_t ConnectionHandler::onNegotiate(AMQP::Connection* connection, uint16_t timeout)
{
    (void)connection;

    log::debug("Negotiated heartbeat interval to {} seconds", timeout);

    // According to https://www.rabbitmq.com/heartbeats.html we actually get the timeout here
    // and we should send a heartbeat every timeout/2. I guess this is an issue in AMQP-CPP

    heartbeat_timer_.start(
        [this](auto error) {
            if (error)
            {
                log::error("heartbeat timer failed: {}", error.message());
                this->onError("heartbeat timer failed");

                return Timer::TimerResult::cancel;
            }

            if (!this->socket_.is_open())
            {
                return Timer::TimerResult::cancel;
            }

            log::debug("Sending heartbeat to server");
            this->connection_->heartbeat();
            return Timer::TimerResult::repeat;
        },
        std::chrono::seconds(timeout / 2));

    return timeout;
}

void ConnectionHandler::onHeartbeat(AMQP::Connection* connection)
{
    (void)connection;

    log::debug("Received heartbeat from server");
}

/**
 *  Method that is called by the AMQP library every time it has data
 *  available that should be sent to RabbitMQ.
 *  @param  connection  pointer to the main connection object
 *  @param  data        memory buffer with the data that should be sent to RabbitMQ
 *  @param  size        size of the buffer
 */
void ConnectionHandler::onData(AMQP::Connection* connection, const char* data, size_t size)
{
    (void)connection;

    log::debug("writing {} bytes", size);

    send_buffers_.emplace(data, size);
    flush();
}

/**
 *  Method that is called by the AMQP library when the login attempt
 *  succeeded. After this method has been called, the connection is ready
 *  to use.
 *  @param  connection      The connection that can now be used
 */
void ConnectionHandler::onReady(AMQP::Connection* connection)
{
    (void)connection;
    log::debug("onReady");
}

/**
 *  Method that is called by the AMQP library when a fatal error occurs
 *  on the connection, for example because data received from RabbitMQ
 *  could not be recognized.
 *  @param  connection      The connection on which the error occurred
 *  @param  message         A human readable error message
 */
void ConnectionHandler::onError(AMQP::Connection* connection, const char* message)
{
    (void)connection;
    log::debug("onError: {}", message);
    socket_.close();

    reconnect_timer_.expires_from_now(std::chrono::seconds(1));
    reconnect_timer_.async_wait([this](const auto& error) {
        if (error)
        {
            log::error("reconnect timer failed: {}", error.message());
            return;
        }

        this->send_buffers_.clear();
        this->recv_buffer_.consume(this->recv_buffer_.size());
        this->flush_in_progress_ = false;
        this->connection_.reset();
        this->heartbeat_timer_.cancel();

        this->connect(*this->address_);
    });
}

/**
 *  Method that is called when the connection was closed. This is the
 *  counter part of a call to Connection::close() and it confirms that the
 *  AMQP connection was correctly closed.
 *
 *  @param  connection      The connection that was closed and that is now unusable
 */
void ConnectionHandler::onClosed(AMQP::Connection* connection)
{
    (void)connection;
    log::debug("onClosed");

    if (!send_buffers_.empty())
    {
        throw std::logic_error("Socket was closed before all data could be sent out.");
    }

    socket_.close();
    connection_.reset();
}

bool ConnectionHandler::close()
{
    connection_->close();
    // FIXME
    return true;
}

void ConnectionHandler::read()
{
    socket_.async_read_some(asio::buffer(recv_buffer_.prepare(connection_->maxFrame() * 32)),
                            [this](const auto& error, auto received_bytes) {
                                if (error)
                                {
                                    log::error("read failed: {}", error.message());
                                    this->onError("read failed");
                                    return;
                                }

                                recv_buffer_.commit(received_bytes);

                                if (recv_buffer_.size() >= connection_->expected())
                                {
                                    std::vector<char> data(asio::buffers_begin(recv_buffer_.data()),
                                                           asio::buffers_end(recv_buffer_.data()));
                                    auto consumed = connection_->parse(data.data(), data.size());
                                    recv_buffer_.consume(consumed);
                                }

                                if (socket_.is_open())
                                {
                                    this->read();
                                }
                            });
}

void ConnectionHandler::flush()
{
    if (!socket_.is_open() || send_buffers_.empty() || flush_in_progress_)
    {
        return;
    }

    flush_in_progress_ = true;

    socket_.async_write_some(send_buffers_.front(), [this](const auto& error, auto transferred) {
        if (error)
        {
            log::error("write failed: {}", error.message());
            this->onError("write failed");
            return;
        }

        log::debug("wrote {} bytes", transferred);

        this->send_buffers_.consume(transferred);

        this->flush_in_progress_ = false;
        this->flush();
    });
}

std::unique_ptr<AMQP::Channel> ConnectionHandler::make_channel()
{
    return std::make_unique<AMQP::Channel>(connection_.get());
}

} // namespace metricq