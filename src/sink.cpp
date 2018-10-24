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
#include <metricq/datachunk.pb.h>
#include <metricq/sink.hpp>
#include <metricq/types.hpp>

#include "log.hpp"
#include "util.hpp"

#include <amqpcpp.h>

#include <iostream>

namespace metricq
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
    if (data_connection_ || data_channel_)
    {
        log::fatal("reconfiguring sink data queue is currently not supported");
        std::abort();
    }

    assert(data_server_address_);
    assert(!data_queue_.empty());
    data_connection_ = std::make_unique<AMQP::TcpConnection>(&data_handler_, *data_server_address_);
    data_channel_ = std::make_unique<AMQP::TcpChannel>(data_connection_.get());
    data_channel_->onError(debug_error_cb("sink data channel error"));
    // Ensure that we are not flooded by requests and forget to send out heartbeat
    // TODO configurable!
    data_channel_->setQos(400);

    data_channel_->declareQueue(data_queue_, AMQP::passive).onSuccess(callback);
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
    try
    {
        data_callback(metric_name, data_chunk_);
    }
    catch (std::exception& ex)
    {
        log::fatal("db data callback failed for metric {}: {}", metric_name, ex.what());
        throw;
    }
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
} // namespace metricq
