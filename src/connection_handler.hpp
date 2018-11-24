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

#pragma once

#include <metricq/timer.hpp>

#include <amqpcpp.h>

#include <asio.hpp>
#include <asio/basic_waitable_timer.hpp>

#include <memory>
#include <optional>
#include <queue>
#include <vector>

namespace metricq
{

class QueuedBuffer
{
public:
    void emplace(const char* ptr, std::size_t size);

    bool empty() const
    {
        return buffers_.empty();
    }

    auto front() const
    {
        assert(!empty());

        return asio::buffer(buffers_.front().data() + offset_, buffers_.front().size() - offset_);
    }

    void consume(std::size_t consumed_bytes);

    void clear()
    {
        buffers_ = decltype(buffers_)();
        offset_ = 0;
    }

private:
    std::queue<std::vector<char>> buffers_;
    std::queue<std::vector<char>> empty_buffers_;
    std::size_t offset_ = 0;
};

class ConnectionHandler : public AMQP::ConnectionHandler
{
public:
    ConnectionHandler(asio::io_service& io_service);

    /**
     *  Method that is called by the AMQP library every time it has data
     *  available that should be sent to RabbitMQ.
     *  @param  connection  pointer to the main connection object
     *  @param  data        memory buffer with the data that should be sent to RabbitMQ
     *  @param  size        size of the buffer
     */
    void onData(AMQP::Connection* connection, const char* data, size_t size) override;

    /**
     *  Method that is called by the AMQP library when the login attempt
     *  succeeded. After this method has been called, the connection is ready
     *  to use.
     *  @param  connection      The connection that can now be used
     */
    void onReady(AMQP::Connection* connection) override;

    void onError(const std::string& message)
    {
        onError(connection_.get(), message.c_str());
    }

    /**
     *  Method that is called by the AMQP library when a fatal error occurs
     *  on the connection, for example because data received from RabbitMQ
     *  could not be recognized.
     *  @param  connection      The connection on which the error occured
     *  @param  message         A human readable error message
     */
    void onError(AMQP::Connection* connection, const char* message) override;

    /**
     *  Method that is called when the connection was closed. This is the
     *  counter part of a call to Connection::close() and it confirms that the
     *  AMQP connection was correctly closed.
     *
     *  @param  connection      The connection that was closed and that is now unusable
     */
    virtual void onClosed(AMQP::Connection* connection) override;

    /**
     *  Method that is called when the heartbeat frequency is negotiated
     *  between the server and the client durion connection setup. You
     *  normally do not have to override this method, because in the default
     *  implementation the suggested heartbeat is simply rejected by the client.
     *
     *  However, if you want to enable heartbeats you can override this
     *  method. You should "return interval" if you want to accept the
     *  heartbeat interval that was suggested by the server, or you can
     *  return an alternative value if you want a shorter or longer interval.
     *  Return 0 if you want to disable heartbeats.
     *
     *  If heartbeats are enabled, you yourself are responsible to send
     *  out a heartbeat every *interval* number of seconds by calling
     *  the Connection::heartbeat() method.
     *
     *  @param  connection      The connection that suggested a heartbeat interval
     *  @param  interval        The suggested interval from the server
     *  @return uint16_t        The interval to use
     */
    virtual uint16_t onNegotiate(AMQP::Connection* connection, uint16_t interval) override;

    /**
     *  Method that is called when the AMQP-CPP library received a heartbeat
     *  frame that was sent by the server to the client.
     *
     *  You do not have to do anything here, the client sends back a heartbeat
     *  frame automatically, but if you like, you can implement/override this
     *  method if you want to be notified of such heartbeats
     *
     *  @param  connection      The connection over which the heartbeat was received
     */
    virtual void onHeartbeat(AMQP::Connection* connection) override;

    bool close();

    void connect(const AMQP::Address& address);

private:
    void connect(asio::ip::tcp::resolver::iterator endpoint_iterator);
    void read();
    void flush();

public:
    std::unique_ptr<AMQP::Channel> make_channel();

private:
    std::unique_ptr<AMQP::Connection> connection_;
    std::optional<AMQP::Address> address_;
    asio::system_timer reconnect_timer_;
    metricq::Timer heartbeat_timer_;
    asio::ip::tcp::resolver resolver_;
    asio::ip::tcp::socket socket_;
    asio::streambuf recv_buffer_;
    QueuedBuffer send_buffers_;
    bool flush_in_progress_ = false;
};
} // namespace metricq
