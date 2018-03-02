#include <dataheap2/connection.hpp>

#include <amqpcpp.h>

#include <nlohmann/json.hpp>

#include <ev.h>

#include <iostream>
#include <memory>
#include <string>

namespace dataheap2
{
const std::string management_queue = "managementQueue";

Connection::Connection(const std::string& connection_token, struct ev_loop* loop)
: handler(loop), connection_token_(connection_token), loop_(loop)
{
    register_management_callback("config", [this](auto& response) { config_callback(response); });
}

Connection::~Connection()
{
}

void Connection::main_loop()
{
    ev_run(loop_, 0);
}

void Connection::connect(const std::string& server_address)
{
    management_connection_ =
        std::make_unique<AMQP::TcpConnection>(&handler, AMQP::Address(server_address));
    management_channel_ = std::make_unique<AMQP::TcpChannel>(management_connection_.get());
    management_channel_->onError([](const char* message) {
        std::cerr << "channel error: " << message << std::endl;
        throw std::runtime_error(message);
    });

    management_channel_
        ->declareQueue(AMQP::exclusive) //  rpc queue
        .onSuccess([this](const std::string& name, int msgcount, int consumercount) {
            management_queue_ = name;

            // callback function that is called when the consume operation starts
            auto startCb = [](const std::string& consumertag) {
                std::cout << "consume operation started" << std::endl;
            };

            // callback function that is called when the consume operation failed
            auto errorCb = [](const char* message) {
                std::cout << "consume operation failed" << std::endl;
            };

            // callback operation when a message was received
            auto messageCb = [this](const AMQP::Message& message, uint64_t deliveryTag,
                                    bool redelivered) {
                std::cout << "message received: " << std::endl;

                dispatch_management(message);

                // acknowledge the message
                management_channel_->ack(deliveryTag);
            };

            management_channel_->consume(name).onReceived(messageCb).onSuccess(startCb).onError(
                errorCb);

            // request initial config
            send_management("register");
            // TODO call subclasses on_init or so
        });
    ;
}

void Connection::register_management_callback(const std::string& function, ManagementCallback cb)
{
    auto ret = management_callbacks_.emplace(function, std::move(cb));
    assert(ret.second);
}

void Connection::send_management(const std::string& function, json payload)
{
    assert(payload.count("function") == 0);
    payload["function"] = function;
    std::string message = payload.dump();
    AMQP::Envelope env(message.data(), message.size());

    env.setAppID(connection_token_);
    assert(!management_queue_.empty());
    env.setReplyTo(management_queue_);

    // publish to default direct exchange
    management_channel_->publish("", management_queue, env);
}

void Connection::dispatch_management(const AMQP::Message& message)
{
    const std::string responseMessage(message.body(), static_cast<size_t>(message.bodySize()));
    try
    {
        management_callbacks_.at(message.correlationID())(json::parse(responseMessage));
    }
    catch (nlohmann::json::parse_error& e)
    {
        std::cerr << "error in rpc response: parsing message: " << e.what() << std::endl;
    }
    catch (nlohmann::json::type_error& e)
    {
        std::cerr << "error in rpc response: accessing parameter: " << e.what() << std::endl;
    }
    catch (std::out_of_range& e)
    {
        std::cerr << "RPC response to unknown correlation id: " << message.correlationID()
                  << std::endl;
    }
}
} // namespace dataheap2
