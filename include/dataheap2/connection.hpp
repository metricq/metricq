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

enum class MessageCoding : char
{
    single = 0x1,
    chunk = 0x2,
};

class Connection
{
public:
    void main_loop();

protected:
    using ManagementCallback = std::function<void(const json& response)>;

    explicit Connection(const std::string& connection_token);
    virtual ~Connection() = 0;

    void connect(const std::string& server_address);

    virtual void config_callback(const json& config) = 0;

    void send_management(const std::string& function, json payload = json());
    void register_management_callback(const std::string& function, ManagementCallback cb);

private:
    void dispatch_management(const AMQP::Message& message);

protected:
    asio::io_service io_service;
    AMQP::LibAsioHandler handler;

private:
    std::string connection_token_;

    // TODO combine & abstract to extra class
    std::unique_ptr<AMQP::TcpConnection> management_connection_;
    std::unique_ptr<AMQP::TcpChannel> management_channel_;
    std::unordered_map<std::string, ManagementCallback> management_callbacks_;
    std::string management_queue_;
};
} // namespace dataheap2
