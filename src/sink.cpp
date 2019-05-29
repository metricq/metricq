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

#include <asio/dispatch.hpp>

#include <amqpcpp.h>

#include <exception>
#include <iostream>

namespace metricq
{
Sink::Sink(const std::string& token, bool add_uuid) : DataClient(token, add_uuid)
{
}

Sink::~Sink()
{
}

void Sink::subscribe(const std::vector<std::string>& metrics, int64_t expires)
{
    rpc("sink.subscribe",
        [this](const json& response) {
            if (this->data_queue_.empty())
            {
                this->sink_config(response);
                this->setup_data_queue();
            }
            if (this->data_queue_ != response.at("dataQueue"))
            {
                throw std::runtime_error("inconsistent sink dataQueue setting after subscription");
            }
        },
        { { "metrics", metrics }, { "expires", expires }, { "metadata", true } });
}

void Sink::sink_config(const json& config)
{
    data_config(config);

    if (data_queue_.empty())
    {
        // Set data queue name from configuration.
        data_queue_ = config.at("dataQueue");
    }
    else
    {
        // If data_queue_ has already been set (for example from Drain), check
        // that the local name matches the one in the configuration provided.
        auto config_data_queue = config.at("dataQueue").get<std::string>();

        if (data_queue_ != config_data_queue)
        {
            log::warn("configuration indicated to use data queue {}, but we expected {}.",
                      config_data_queue, data_queue_);
            log::warn("using queue provided by configuration");
            data_queue_ = config_data_queue;
        }
    }

    if (config.count("metrics"))
    {
        const auto& metrics_metadata = config.at("metrics");
        for (auto it = metrics_metadata.begin(); it != metrics_metadata.end(); ++it)
        {
            if (it.value().is_object())
            {
                metadata_.emplace(it.key(), it.value());
            }
            else
            {
                log::warn("missing metadata for metric {}", it.key());
            }
        }
    }
}

void Sink::setup_data_queue()
{
    setup_data_queue([this](const std::string& name, int message_count, int consumer_count) {
        log::notice("setting up data queue, messages {}, consumers {}", message_count,
                    consumer_count);
        // we do not tolerate other consumers
        if (consumer_count != 0)
        {
            log::fatal("unexpected consumer count {} - are we not alone in the queue?",
                       consumer_count);
        }

        auto message_cb = [this](const AMQP::Message& message, uint64_t delivery_tag,
                                 bool redelivered) { on_data(message, delivery_tag, redelivered); };

        data_channel_->consume(name)
            .onReceived(message_cb)
            .onSuccess(debug_success_cb("sink data queue consume success"))
            .onError(debug_error_cb("sink data queue consume error"))
            .onFinalize([]() { log::info("sink data queue consume finalize"); });
    });
}

void Sink::setup_data_queue(const AMQP::QueueCallback& callback)
{
    // Ensure that we are not flooded by requests and forget to send out heartbeat
    // TODO configurable!
    data_channel_->setQos(400);
    assert(!data_queue_.empty());
    data_channel_->declareQueue(data_queue_, AMQP::passive).onSuccess(callback);
}

void Sink::on_data(const AMQP::Message& message, uint64_t delivery_tag, bool redelivered)
{
    (void)redelivered;
    const auto& metric_name = message.routingkey();
    auto message_string = std::string(message.body(), message.bodySize());
    data_chunk_.Clear();
    data_chunk_.ParseFromString(message_string);
    try
    {
        on_data(metric_name, std::move(data_chunk_), DataCompletion(*this, delivery_tag));
    }
    catch (std::exception& ex)
    {
        log::fatal("sink data callback failed for metric {}: {}", metric_name, ex.what());
        throw;
    }
}

// Default implementation with immediate completion
void Sink::on_data(const std::string& id, const DataChunk& data_chunk, DataCompletion complete)
{
    on_data(id, data_chunk);
    complete();
}

void Sink::on_data(const std::string& id, const DataChunk& data_chunk)
{
    for (auto tv : data_chunk)
    {
        on_data(id, tv);
    }
}

void Sink::on_data(const std::string&, TimeValue)
{
    log::fatal("unhandled TimeValue data, implementation error.");
    std::abort();
}

void Sink::DataCompletion::operator()()
{
    auto run = [& self = this->self, delivery_tag = this->delivery_tag]() {
        self.data_channel_->ack(delivery_tag);
    };
    asio::dispatch(self.io_service, run);
}
} // namespace metricq
