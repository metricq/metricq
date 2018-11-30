// Copyright (c) 2018, ZIH,
// Technische Universitaet Dresden,
// Federal Republic of Germany
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

#include <asio/io_service.hpp>

#include <amqpcpp.h>

#include <metricq/asio_handler.hpp>

#include <nlohmann/json.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace metricq
{
using json = nlohmann::json;

class ConnectionHandler;

class Connection
{
public:
    void main_loop();

protected:
    using ManagementCallback = std::function<json(const json& response)>;
    using ManagementResponseCallback = std::function<void(const json& response)>;

    explicit Connection(const std::string& connection_token, bool add_uuid = false,
                        std::size_t concurrency_hint = 1);
    virtual ~Connection() = 0;

public:
    void connect(const std::string& server_address);

public:
    virtual void on_error(const char* message)
    {
        (void)message;
    }

    virtual void on_lost()
    {
    }

    virtual void on_detached()
    {
    }

protected:
    virtual void on_connected() = 0;
    virtual void on_error(const std::string& message)
    {
        (void)message;
    }

    void rpc(const std::string& function, ManagementResponseCallback callback,
             json payload = json({}));
    void register_management_callback(const std::string& function, ManagementCallback callback);

    void stop();
    virtual void close();

    AMQP::Address add_credentials(const AMQP::Address& address);

private:
    void handle_management_message(const AMQP::Message& incoming_message, uint64_t deliveryTag,
                                   bool redelivered);
    void handle_broadcast_message(const AMQP::Message& message);

protected:
    asio::io_service io_service;

private:
    std::optional<AMQP::Address> management_address_;
    std::string connection_token_;

    // TODO combine & abstract to extra class
    std::unique_ptr<ConnectionHandler> management_connection_;
    std::unique_ptr<AMQP::Channel> management_channel_;
    std::unordered_map<std::string, ManagementCallback> management_callbacks_;
    std::unordered_map<std::string, ManagementResponseCallback> management_rpc_response_callbacks_;
    std::string management_client_queue_;
    std::string management_queue_ = "management";
    std::string management_exchange_ = "metricq.management";
    std::string management_broadcast_exchange_ = "metricq.broadcast";
};
} // namespace metricq
