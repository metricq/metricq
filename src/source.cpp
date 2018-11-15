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

#include "log.hpp"

#include <metricq/datachunk.pb.h>
#include <metricq/source.hpp>

#include <amqpcpp.h>

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

namespace metricq
{

Source::Source(const std::string& token) : DataClient(token)
{
}

void Source::on_connected()
{
    rpc("source.register", [this](const auto& response) { config(response); });
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

void Source::config(const json& config)
{
    data_config(config);

    if (!data_exchange_.empty() && config["dataExchange"] != data_exchange_)
    {
        log::fatal("changing dataExchange on the fly is not currently supported");
        std::abort();
    }

    data_exchange_ = config["dataExchange"];

    if (config.find("config") != config.end())
    {
        on_source_config(config["config"]);
    }
}

void Source::on_data_channel_ready()
{
    on_source_ready();
    declare_metrics();
}

void Source::declare_metrics()
{
    if (metrics_.empty())
    {
        return;
    }

    json payload;
    for (auto& metric : metrics_)
    {
        payload["metrics"].push_back(metric.second.id());
    }
    rpc("source.declare_metrics", [this](const auto&) { /* nothing to do */ (void)this; }, payload);
}
} // namespace metricq
