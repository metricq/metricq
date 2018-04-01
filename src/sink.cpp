#include <dataheap2/datachunk.pb.h>
#include <dataheap2/sink.hpp>
#include <dataheap2/types.hpp>

#include "log.hpp"
#include "util.hpp"

#include <amqpcpp.h>

#include <iostream>

namespace dataheap2
{
Sink::Sink(const std::string& token, bool add_uuid)
: Connection(token, add_uuid), data_handler_(io_service)
{
}

Sink::~Sink()
{
}

void Sink::setup_data_queue(const AMQP::QueueCallback& callback)
{
    assert(!data_server_address_.empty());
    assert(!data_queue_.empty());
    data_connection_ =
        std::make_unique<AMQP::TcpConnection>(&data_handler_, AMQP::Address(data_server_address_));
    data_channel_ = std::make_unique<AMQP::TcpChannel>(data_connection_.get());
    data_channel_->onError(debug_error_cb("sink data channel error"));

    data_channel_->declareQueue(data_queue_).onSuccess(callback);
}

/*
void Sink::config_callback(const json& config)
{
    std::cerr << "sink parsing config" << std::endl;

    if (data_connection_)
    {
        if (config["dataServerAddress"] != data_server_address_)
        {
            std::cerr << "FATAL: Changing dataServerAddress on the fly is not currently supported."
                      << std::endl;
            std::abort();
        }
        if (config["dataQueue"] != data_queue_)
        {
            std::cerr << "FATAL: Changing dataQueue on the fly is not currently supported."
                      << std::endl;
            std::abort();
        }
    }

    const std::string& data_server_address_ = config["dataServerAddress"];
    data_queue_ = config["dataQueue"];

    data_connection_ =
        std::make_unique<AMQP::TcpConnection>(&handler, AMQP::Address(data_server_address_));
    data_channel_ = std::make_unique<AMQP::TcpChannel>(data_connection_.get());
    data_channel_->onError(
        [](const char* message) { std::cerr << "data channel error: " << message << std::endl; });

    data_channel_->declareQueue(data_queue_)
        .onSuccess([this](const std::string& name, int msgcount, int consumercount) {
            std::cerr << "setting up sink queue. msgcount: " << msgcount << ", consumercount"
                      << consumercount << std::endl;

            auto start_cb = [](const std::string& consumertag) {
                std::cerr << "data consume operation started: " << consumertag << std::endl;
            };

            auto error_cb = [](const char* message) {
                std::cerr << "data consume operation failed: " << message << std::endl;
            };

            auto message_cb = [this](const AMQP::Message& message, uint64_t deliveryTag,
                                     bool redelivered) {
                (void)redelivered;
                data_callback(message);
                data_channel_->ack(deliveryTag);
            };

            data_channel_->consume(name)
                .onReceived(message_cb)
                .onSuccess(start_cb)
                .onError(error_cb);
        });

    if (config.find("sinkConfig") != config.end())
    {
        sink_config_callback(config["sinkConfig"]);
    }
    ready_callback();
}
*/

void Sink::data_callback(const AMQP::Message& message)
{
    const auto& metric_name = message.routingkey();
    auto message_string = std::string(message.body(), message.bodySize());
    data_chunk_.Clear();
    data_chunk_.ParseFromString(message_string);
    data_callback(metric_name, data_chunk_);
}

void Sink::data_callback(const std::string&, TimeValue)
{
}

void Sink::data_callback(const std::string& id, const DataChunk& data_chunk)
{
    for (auto tv : data_chunk)
    {
        data_callback(id, tv);
    }
}

void Sink::close()
{
    Connection::close();
    if (!data_connection_)
    {
        log::debug("closing sink, no data_connection up yet");
        return;
    }
    auto alive = data_connection_->close();
    log::info("closed sink data connection: {}", alive);
}
} // namespace dataheap2
