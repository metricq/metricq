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
#include <metricq/source.hpp>

#include "log.hpp"

#include <metricq/datachunk.pb.h>

#include <amqpcpp.h>

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

namespace metricq
{

Source::Source(const std::string& token) : Connection(token), data_handler_(io_service)
{
    TimePoint starting_time = Clock::now();
    register_management_callback("discover", [token, starting_time](const json&) {
        json response;
        response["alive"] = true;
        response["currentTime"] = Clock::format_iso(Clock::now());
        response["startingTime"] = Clock::format_iso(starting_time);
        return response;
    });
}

Source::~Source()
{
}

void Source::setup_complete()
{
    rpc("source.register", [this](const auto& config) { config_callback(config); });
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
    AMQP::Address new_data_server_address =
        add_credentials(config["dataServerAddress"].get<std::string>());
    log::debug("start parsing source config");
    if (data_connection_)
    {
        if (new_data_server_address != data_server_address_)
        {
            log::fatal("changing dataServerAddress on the fly is not currently supported");
            std::abort();
        }
        if (config["dataExchange"] != data_exchange_)
        {
            log::fatal("changing dataQueue on the fly is not currently supported");
            std::abort();
        }
    }

    data_server_address_ = new_data_server_address;
    data_exchange_ = config["dataExchange"];

    data_connection_ =
        std::make_unique<AMQP::TcpConnection>(&data_handler_, new_data_server_address);
    data_channel_ = std::make_unique<AMQP::TcpChannel>(data_connection_.get());
    data_channel_->onError([](const char* message) {
        // report error
        log::error("data channel error: {}", message);
    });

    if (config.find("config") != config.end())
    {
        source_config_callback(config["config"]);
    }
    send_metrics_list();
    ready_callback();
}

void Source::send_metrics_list()
{
    json payload;
    for (auto& metric : metrics_)
    {
        payload["metrics"].push_back(metric.second.id());
    }
    rpc("source.metrics_list", [this](const auto&) { /* nothing to do */ }, payload);
}

void Source::close()
{
    Connection::close();
    if (!data_connection_)
    {
        log::debug("closing source, no data_connection up yet");
        return;
    }
    auto alive = data_connection_->close();
    log::info("closed source data connection: {}", alive);
}

void SourceMetric::flush()
{
    source_.send(id_, chunk_);
    chunk_.clear_time_delta();
    chunk_.clear_value();
    previous_timestamp_ = 0;
}

void SourceMetric::send(TimeValue tv)
{
    chunk_.add_time_delta(tv.time.time_since_epoch().count() - previous_timestamp_);
    previous_timestamp_ = tv.time.time_since_epoch().count();
    chunk_.add_value(tv.value);

    assert(chunk_.time_delta_size() == chunk_.value_size());
    if (chunk_size_ && chunk_.time_delta_size() == chunk_size_)
    {
        flush();
    }
}

} // namespace metricq
