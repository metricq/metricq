#include <dataheap2/connection.hpp>

#include "util.hpp"

#include <amqpcpp.h>

#include <nlohmann/json.hpp>

#include <iostream>
#include <memory>
#include <string>

namespace dataheap2
{
const std::string management_queue = "dh2.management";

Connection::Connection(const std::string& connection_token, std::size_t concurrency_hint)
: io_service(concurrency_hint), handler(io_service), connection_token_(connection_token)
{
}

Connection::~Connection()
{
}

void Connection::main_loop()
{
    io_service.run();
}

static auto debug_error_cb(const std::string& prefix)
{
    return [prefix](const auto& message) { std::cerr << prefix << ": " << message << std::endl; };
}

static auto debug_success_cb(const std::string& prefix)
{
    return [prefix]() { std::cerr << prefix << std::endl; };
}

void Connection::connect(const std::string& server_address)
{
    std::cerr << "connecting to management server: " << server_address << std::endl;
    management_connection_ =
        std::make_unique<AMQP::TcpConnection>(&handler, AMQP::Address(server_address));
    management_channel_ = std::make_unique<AMQP::TcpChannel>(management_connection_.get());
    management_channel_->onError(debug_error_cb("management channel error"));

    management_queue_ = std::string("client-") + connection_token_ + "-rpc";

    management_channel_->declareQueue(management_queue, AMQP::exclusive)
        .onSuccess([this](const std::string& name, int msgcount, int consumercount) {
            management_queue_ = name;
            management_channel_->bindQueue(management_broadcast_exchange_, management_queue_, "#")
                .onError(debug_error_cb("error binding management queue to broadcast exchange"))
                .onSuccess([this, name]() {
                    management_channel_->consume(name)
                        .onReceived([this](const AMQP::Message& message, uint64_t delivery_tag,
                                           bool redelivered) {
                            handle_management_message(message, delivery_tag, redelivered);
                        })
                        .onSuccess([this]() { setup_complete(); })
                        .onError(debug_error_cb("management consume error"));
                });
        });
}

void Connection::register_management_callback(const std::string& function, ManagementCallback cb)
{
    auto ret = management_callbacks_.emplace(function, std::move(cb));
    assert(ret.second);
}

void Connection::rpc(const std::string& function, ManagementResponseCallback response_callback,
                     json payload)
{
    std::string message = payload.dump();
    AMQP::Envelope env(message.data(), message.size());

    auto correlation_id = uuid(std::string("dh2-rpc-") + connection_token_ + "-");
    env.setCorrelationID(correlation_id);
    env.setAppID(connection_token_);
    assert(!management_queue_.empty());
    env.setReplyTo(management_queue_);

    auto ret =
        management_rpc_response_callbacks_.emplace(correlation_id, std::move(response_callback));
    assert(ret.second);
    management_channel_->publish(management_broadcast_exchange_, function, env);
}

void Connection::handle_management_message(const AMQP::Message& incoming_message,
                                           uint64_t deliveryTag, bool redelivered)
{
    std::cerr << "management message received" << std::endl;

    const std::string content_str(incoming_message.body(),
                                  static_cast<size_t>(incoming_message.bodySize()));
    try
    {
        auto content = json::parse(content_str);

        if (auto it = management_rpc_response_callbacks_.find(incoming_message.correlationID());
            it != management_rpc_response_callbacks_.end())
        {
            // Incoming message is a RPC-response, call the response handler
            it->second(content);
        }
        else if (auto it = management_callbacks_.find(incoming_message.routingkey());
                 it != management_callbacks_.end())
        {
            // incoming message is a RPC-call
            auto response = it->second(content);

            std::string reply_message; // = response.dump();
            AMQP::Envelope env(reply_message.data(), reply_message.size());
            env.setCorrelationID(incoming_message.correlationID());
            env.setAppID(connection_token_);
            management_channel_->publish("", incoming_message.replyTo(), env);
        }
        else
        {
            std::cerr << "message not found as rpc response or callback." << std::endl;
        };
    }
    catch (nlohmann::json::parse_error& e)
    {
        std::cerr << "error in rpc response: parsing message: " << e.what() << std::endl;
    }
    catch (nlohmann::json::type_error& e)
    {
        std::cerr << "error in rpc response: accessing parameter: " << e.what() << std::endl;
    }

    management_channel_->ack(deliveryTag);
}

void Connection::close()
{
    if (!management_connection_)
    {
        std::cerr << "closing connection, no management_connection up yet." << std::endl;
        return;
    }
    auto alive = management_connection_->close();
    std::cerr << "closed management_connection: " << alive << "\n";
}

void Connection::stop()
{
    std::cerr << "requesting stop." << std::endl;
    close();
    std::cerr << "stopping io_service." << std::endl;
    // the io_service will stop itself once all connections are closed
}
} // namespace dataheap2
