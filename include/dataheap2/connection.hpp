#pragma once

#include <asio/io_service.hpp>

#include <amqpcpp.h>
#include <amqpcpp/libasio.h>

#include <nlohmann/json.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace dataheap2
{
using json = nlohmann::json;

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

protected:
    virtual void setup_complete() = 0;

    void rpc(const std::string& function, ManagementResponseCallback callback,
             json payload = json({}));
    void register_management_callback(const std::string& function, ManagementCallback callback);

    void stop();
    virtual void close();

private:
    void handle_management_message(const AMQP::Message& incoming_message, uint64_t deliveryTag,
                                   bool redelivered);
    void handle_broadcast_message(const AMQP::Message& message);

protected:
    asio::io_service io_service;

private:
    std::string connection_token_;

    // TODO combine & abstract to extra class
    AMQP::LibAsioHandler management_handler_;
    std::unique_ptr<AMQP::TcpConnection> management_connection_;
    std::unique_ptr<AMQP::TcpChannel> management_channel_;
    std::unordered_map<std::string, ManagementCallback> management_callbacks_;
    std::unordered_map<std::string, ManagementResponseCallback> management_rpc_response_callbacks_;
    std::string management_client_queue_;
    std::string management_queue_ = "management";
    std::string management_exchange_ = "dh2.management";
    std::string management_broadcast_exchange_ = "dh2.broadcast";
};
} // namespace dataheap2
