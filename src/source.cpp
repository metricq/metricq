#include <dataheap2/source.hpp>

#include <protobufmessages/datachunk.pb.h>
#include <protobufmessages/datapoint.pb.h>

#include <amqpcpp.h>

#include <nlohmann/json.hpp>

#include <ev.h>

#include <iostream>
#include <memory>
#include <string>

#include "ev++.h"

namespace dataheap2
{

Source::Source(const std::string& token, struct ev_loop* loop) : Connection(token, loop)
{
}

Source::~Source()
{
}

void Source::send(const std::string& id, const DataChunk& dc)
{
    data_channel_->publish(data_exchange_, id, dc.SerializeAsString());
}

void Source::send(const std::string& id, TimeValue tv)
{
    // TODO evaluate optimization of string construction
    data_channel_->publish(data_exchange_, id, DataChunk(tv).SerializeAsString());
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

void Source::__timer_callback(ev::timer&, int)
{
    timer_callback_();
}

void Source::register_timer(std::function<void()> callback, Duration duration)
{
    if (timer_)
    {
        std::cerr << "One timer already registerd. Can only have one :(" << std::endl;
        std::abort();
    }

    auto interval = std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();

    std::cout << "Register a timer callback every " << interval << " seconds." << std::endl;

    timer_ = std::make_unique<ev::timer>(loop_);

    timer_callback_ = callback;

    timer_->set<Source, &Source::__timer_callback>(this);
    timer_->start(0, interval);
}

void SourceMetric::flush()
{
    source_.send(id_, chunk_);
    chunk_.clear_data();
}

void SourceMetric::send(TimeValue tv)
{
    if (chunk_size_ == 0)
    {
        source_.send(id_, tv);
    }
    else
    {
        if (chunk_.data_size() == 0)
        {
            chunk_.set_timestamp_offset(tv.time.time_since_epoch().count());
        }
        auto new_data = chunk_.add_data();
        new_data->set_timestamp(tv.time.time_since_epoch().count() - chunk_.timestamp_offset());
        new_data->set_value(tv.value);
        if (chunk_.data_size() == chunk_size_)
        {
            flush();
        }
    }
}
} // namespace dataheap2
