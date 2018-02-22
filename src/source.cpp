#include <dataheap2/source.hpp>

#include <protobufmessages/datachunk.pb.h>
#include <protobufmessages/datapoint.pb.h>

#include <amqpcpp.h>

#include <nlohmann/json.hpp>

#include <ev.h>

#include <iostream>
#include <memory>
#include <string>

namespace dataheap2
{

Source::Source(const std::string& token, struct ev_loop* loop) : Connection(token, loop)
{
}

void Source::send(const std::string& id, const DataChunk& dc)
{
    std::string chunk_string(1, static_cast<char>(MessageCoding::chunk));
    chunk_string += dc.SerializeAsString();
    data_channel_->publish(data_exchange_, id, chunk_string);
}

void Source::send(const std::string& id, TimeValue tv)
{
    std::string datapoint_string(1, static_cast<char>(MessageCoding::single));
    datapoint_string += DataPoint(tv).SerializeAsString();
    // TODO evaluate optimization of string construction
    data_channel_->publish(data_exchange_, id, datapoint_string);
}

void Source::config_callback(const nlohmann::json& config)
{
    std::cout << "Start parsing config" << std::endl;
    if (data_connection_)
    {
        if (config["dataServerAddress"] != data_server_address_)
        {
            std::cerr << "Changing dataServerAddress on the fly is not currently supported.\n";
            std::abort();
        }
        if (config["dataQueue"] != data_exchange_)
        {
            std::cerr << "Changing dataQueue on the fly is not currently supported.\n";
            std::abort();
        }
    }

    data_server_address_ = config["dataServerAddress"];
    data_exchange_ = config["dataExchange"];

    data_connection_ =
        std::make_unique<AMQP::TcpConnection>(&handler, AMQP::Address(data_server_address_));
    data_channel_ = std::make_unique<AMQP::TcpChannel>(data_connection_.get());
    data_channel_->onError([](const char* message) {
        // report error
        std::cout << "data channel error: " << message << std::endl;
    });

    if (config.find("sourceConfig") != config.end())
    {
        source_config_callback(config["sourceConfig"]);
    }
    ready_callback();
}

void SourceMetric::flush()
{
    source_.send(id_, chunk_);
    chunk_.clear_data();
}

void SourceMetric::operator<<(TimeValue tv)
{
    if (chunk_size_ == 0)
    {
        source_.send(id_, tv);
    }
    else
    {
        auto new_data = chunk_.add_data();
        new_data->set_timestamp(tv.time.time_since_epoch().count());
        new_data->set_value(tv.value);
        if (chunk_.data_size() == chunk_size_)
        {
            flush();
        }
    }
}
}
