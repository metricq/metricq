#include <dataheap2/sink.hpp>
#include <dataheap2/types.hpp>

#include <protobufmessages/datachunk.pb.h>
#include <protobufmessages/datapoint.pb.h>

#include <amqpcpp.h>

#include <nlohmann/json.hpp>

#include <iostream>

namespace dataheap2
{
using json = nlohmann::json;

Sink::Sink(const std::string& token, struct ev_loop* loop) : Connection(token, loop)
{
}

void Sink::config_callback(const json& config)
{
    std::cout << "Start parsing config" << std::endl;

    if (data_connection_)
    {
        if (config["dataServerAddress"] != data_server_address_)
        {
            std::cerr << "Changing dataServerAddress on the fly is not currently supported.\n";
            std::abort();
        }
        if (config["dataQueue"] != data_queue_)
        {
            std::cerr << "Changing dataQueue on the fly is not currently supported.\n";
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

    data_channel_
        ->declareQueue(data_queue_) //  rpc queue
        .onSuccess([this](const std::string& name, int msgcount, int consumercount) {
            (void)msgcount;
            (void)consumercount;

            // callback function that is called when the consume operation starts
            auto startCb = [](const std::string& consumertag) {
                (void)consumertag;
                std::cout << "consume operation started" << std::endl;
            };

            // callback function that is called when the consume operation failed
            auto errorCb = [](const char* message) {
                (void)message;
                std::cerr << "consume operation failed" << std::endl;
            };

            // callback operation when a message was received
            auto messageCb = [this](const AMQP::Message& message, uint64_t deliveryTag,
                                    bool redelivered) {
                (void)redelivered;
                data_callback(message);
                data_channel_->ack(deliveryTag);
            };

            data_channel_->consume(name).onReceived(messageCb).onSuccess(startCb).onError(errorCb);
        });

    if (config.find("sinkConfig") != config.end())
    {
        sink_config_callback(config["sinkConfig"]);
    }
    ready_callback();
}

void Sink::data_callback(const AMQP::Message& message)
{
    const auto& metric_name = message.routingkey();
    auto message_string = std::string(message.body(), message.bodySize());
    datachunk_.Clear();
    datachunk_.ParseFromString(message_string);
    data_callback(metric_name, datachunk_);
}

void Sink::data_callback(const std::string& id, const DataChunk& data_chunk)
{
    if(data_chunk.has_value()) {
      data_callback(id,
                    { TimePoint(Duration(data_chunk.timestamp_offset())), data_chunk.value() });
      return;
    }

    auto offset = data_chunk.timestamp_offset();
    for (const auto& data_point : data_chunk.data())
    {
        offset += data_point.timestamp();
        data_callback(id,
                      { TimePoint(Duration(offset)), data_point.value() });
    }
}
}
