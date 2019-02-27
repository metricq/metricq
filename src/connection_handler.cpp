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

#ifdef METRICQ_SSL_SKIP_VERIFY
extern "C"
{
#include <openssl/x509.h>
}
#endif

namespace metricq
{

void QueuedBuffer::emplace(const char* ptr, std::size_t size)
{
    // check if this fits at the back of the last queued buffer
    buffers_.emplace(ptr, ptr + size);
}

void QueuedBuffer::consume(std::size_t consumed_bytes)
{
    assert(!empty());

    if (buffers_.front().size() == offset_ + consumed_bytes)
    {
        offset_ = 0;
        buffers_.pop();
    }
    else
    {
        offset_ += consumed_bytes;
    }
}

BaseConnectionHandler::BaseConnectionHandler(asio::io_service& io_service)
: reconnect_timer_(io_service), heartbeat_timer_(io_service),
  heartbeat_interval_(std::chrono::seconds(0)), resolver_(io_service)
{
}

ConnectionHandler::ConnectionHandler(asio::io_service& io_service)
: BaseConnectionHandler(io_service), socket_(io_service)
{
    log::debug("Using plaintext connection.");
}

SSLConnectionHandler::SSLConnectionHandler(asio::io_service& io_service)
: BaseConnectionHandler(io_service), ssl_context_(asio::ssl::context::tls),
  socket_(io_service, ssl_context_)
{
    log::debug("Using SSL-secured connection.");

    // Create a context that uses the default paths for finding CA certificates.
    ssl_context_.set_default_verify_paths();
    socket_.set_verify_mode(asio::ssl::verify_peer);
    ssl_context_.set_options(asio::ssl::context::default_workarounds |
                             asio::ssl::context::no_sslv2 | asio::ssl::context::no_sslv3 |
                             asio::ssl::context::tlsv12_client);
}

void BaseConnectionHandler::connect(const AMQP::Address& address)
{
    assert(!underlying_socket().is_open());
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

void BaseConnectionHandler::connect(asio::ip::tcp::resolver::iterator endpoint_iterator)
{
    asio::async_connect(this->underlying_socket(), endpoint_iterator,
                        [this](const auto& error, auto successful_endpoint) {
                            if (error)
                            {
                                log::error("Failed to connect to: {}", error.message());
                                this->onError("Connect failed");
                                return;
                            }
                            log::debug("Established connection to {} at {}",
                                       successful_endpoint->host_name(),
                                       successful_endpoint->endpoint());

                            this->handshake(successful_endpoint->host_name());
                        });
}

void ConnectionHandler::handshake(const std::string& hostname)
{
    (void)hostname;

    this->read();
    this->flush();
}

void SSLConnectionHandler::handshake(const std::string& hostname)
{
#ifdef METRICQ_SSL_SKIP_VERIFY
    // Building without SSL verification; DO NOT USE THIS IN PRODUCTION!
    // This code will skip ANY SSL certificate verification.
    socket_.set_verify_callback([](bool, asio::ssl::verify_context& ctx) {
        char subject_name[256];
        X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
        X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);

        log::debug("Visiting certificate for subject: {}", subject_name);

        log::warn("Skipping certificate verification.");
        return true;
    });
#else
    // This code will do proper SSL certification as described in RFC2818
    socket_.set_verify_callback(asio::ssl::rfc2818_verification(hostname));
#endif

    socket_.async_handshake(asio::ssl::stream_base::client, [this](const auto& error) {
        if (error)
        {
            log::error("Failed to SSL handshake to: {}", error.message());
            this->onError("SSL handshake failed");
            return;
        }

        log::debug("SSL handshake was successful.");

        this->read();
        this->flush();
    });
}

uint16_t BaseConnectionHandler::onNegotiate(AMQP::Connection* connection, uint16_t timeout)
{
    (void)connection;

    log::debug("Negotiated heartbeat interval to {} seconds", timeout);

    // According to https://www.rabbitmq.com/heartbeats.html we actually get the timeout here
    // and we should send a heartbeat every timeout/2. I guess this is an issue in AMQP-CPP
    heartbeat_interval_ = std::chrono::seconds(timeout / 2);

    // We don't need to setup the heartbeat timer here. The library will report back our returned
    // timeout value to the server, which inevitable will trigger a send operation, which will
    // setup the heartbeat timer once it was completed. So we could setup the timer here, but it
    // would be canceled anyway.

    return timeout;
}

void BaseConnectionHandler::onHeartbeat(AMQP::Connection* connection)
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
void BaseConnectionHandler::onData(AMQP::Connection* connection, const char* data, size_t size)
{
    (void)connection;

    send_buffers_.emplace(data, size);
    flush();
}

/**
 *  Method that is called by the AMQP library when the login attempt
 *  succeeded. After this method has been called, the connection is ready
 *  to use.
 *  @param  connection      The connection that can now be used
 */
void BaseConnectionHandler::onReady(AMQP::Connection* connection)
{
    (void)connection;
    log::debug("ConnectionHandler::onReady");
}

/**
 *  Method that is called by the AMQP library when a fatal error occurs
 *  on the connection, for example because data received from RabbitMQ
 *  could not be recognized.
 *  @param  connection      The connection on which the error occurred
 *  @param  message         A human readable error message
 */
void BaseConnectionHandler::onError(AMQP::Connection* connection, const char* message)
{
    (void)connection;
    log::debug("ConnectionHandler::onError: {}", message);
    if (error_callback_)
    {
        error_callback_(message);
    }

    underlying_socket().close();

    // We make a hard cut: Just throw the shit out of here.
    throw std::runtime_error(std::string("ConnectionHandler::onError: ") + message);

    // TODO actually implement reconnect
    // reconnect_timer_.expires_from_now(std::chrono::seconds(3));
    // reconnect_timer_.async_wait([this](const auto& error) {
    //     if (error)
    //     {
    //         log::error("reconnect timer failed: {}", error.message());
    //         return;
    //     }
    //
    //     this->send_buffers_.clear();
    //     this->recv_buffer_.consume(this->recv_buffer_.size());
    //     this->flush_in_progress_ = false;
    //     this->connection_.reset();
    //     this->heartbeat_timer_.cancel();
    //
    //     this->connect(*this->address_);
    // });
}

/**
 *  Method that is called when the connection was closed. This is the
 *  counter part of a call to Connection::close() and it confirms that the
 *  AMQP connection was correctly closed.
 *
 *  @param  connection      The connection that was closed and that is now unusable
 */
void BaseConnectionHandler::onClosed(AMQP::Connection* connection)
{
    (void)connection;
    log::debug("ConnectionHandler::onClosed");

    // Technically, there is a ssl_shutdown method for the stream.
    // But, it seems that we don't have to do that (⊙.☉)7
    // If we try, the shutdown errors with a "connection reset by peer"
    // ... instead we're just closing the socket ¯\_(ツ)_/¯
    underlying_socket().close();

    // cancel the heartbeat timer
    heartbeat_timer_.cancel();

    // check if send_buffers are empty, this would be strange, because that means that AMQP-CPP
    // tried to sent something, after it sent the close frame.
    assert(send_buffers_.empty());

    // this technically invalidates all existing channel objects. Those objects are the hard part
    // for a robust connection
    connection_.reset();

    if (close_callback_)
    {
        close_callback_();
    }
}

// TODO check if this return code makes any sense
bool BaseConnectionHandler::close()
{
    if (!connection_)
    {
        return false;
    }
    connection_->close();
    return true;
}

void BaseConnectionHandler::read()
{
    this->async_read_some([this](const auto& error, auto received_bytes) {
        if (error)
        {
            log::error("read failed: {}", error.message());
            if (this->connection_)
            {
                this->connection_->fail(error.message().c_str());
            }
            this->onError("read failed");
            return;
        }

        assert(this->connection_);
        this->recv_buffer_.commit(received_bytes);

        if (this->recv_buffer_.size() >= connection_->expected())
        {
            std::vector<char> data(asio::buffers_begin(this->recv_buffer_.data()),
                                   asio::buffers_end(this->recv_buffer_.data()));
            auto consumed = connection_->parse(data.data(), data.size());
            this->recv_buffer_.consume(consumed);
        }

        if (this->underlying_socket().is_open())
        {
            this->read();
        }
    });
}

void BaseConnectionHandler::flush()
{
    if (!underlying_socket().is_open() || send_buffers_.empty() || flush_in_progress_)
    {
        return;
    }

    flush_in_progress_ = true;

    this->async_write_some([this](const auto& error, auto transferred) {
        if (error)
        {
            log::error("write failed: {}", error.message());
            if (this->connection_)
            {
                this->connection_->fail(error.message().c_str());
            }
            this->onError("write failed");
            return;
        }

        this->send_buffers_.consume(transferred);

        if (heartbeat_interval_.count() != 0 && this->underlying_socket().is_open())
        {
            // we completed a send operation. Now it's time to set up the heartbeat timer.
            this->heartbeat_timer_.expires_after(this->heartbeat_interval_);
            this->heartbeat_timer_.async_wait([this](const auto& error) { this->beat(error); });
        }

        this->flush_in_progress_ = false;
        this->flush();
    });
}

void ConnectionHandler::async_write_some(std::function<void(std::error_code, std::size_t)> cb)
{
    socket_.async_write_some(send_buffers_.front(), cb);
}

void ConnectionHandler::async_read_some(std::function<void(std::error_code, std::size_t)> cb)
{
    assert(this->connection_);
    socket_.async_read_some(asio::buffer(recv_buffer_.prepare(connection_->maxFrame() * 32)), cb);
}

void SSLConnectionHandler::async_write_some(std::function<void(std::error_code, std::size_t)> cb)
{
    socket_.async_write_some(send_buffers_.front(), cb);
}

void SSLConnectionHandler::async_read_some(std::function<void(std::error_code, std::size_t)> cb)
{
    assert(this->connection_);
    socket_.async_read_some(asio::buffer(recv_buffer_.prepare(connection_->maxFrame() * 32)), cb);
}

void BaseConnectionHandler::beat(const asio::error_code& error)
{
    if (error && error == asio::error::operation_aborted)
    {
        // timer was canceled, this is probably fine
        return;
    }

    if (error && error != asio::error::operation_aborted)
    {
        // something weird did happen
        log::error("heartbeat timer failed: {}", error.message());
        if (this->connection_)
        {
            connection_->fail(error.message().c_str());
        }
        onError("heartbeat timer failed");

        return;
    }

    log::debug("Sending heartbeat to server");
    assert(this->connection_);
    connection_->heartbeat();

    // We don't need to setup the timer again, this will be done by the flush() handler triggerd by
    // the heartbeat() call itself
}

std::unique_ptr<AMQP::Channel> BaseConnectionHandler::make_channel()
{
    assert(this->connection_);
    return std::make_unique<AMQP::Channel>(connection_.get());
}

} // namespace metricq
