// Copyright (c) 2018, ZIH, Technische Universitaet Dresden, Federal Republic of Germany
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

#include "dummy_transformer.hpp"

#include <metricq/logger/nitro.hpp>

#include <metricq/types.hpp>

#include <chrono>

#include <cmath>

using Log = metricq::logger::nitro::Log;

DummyTransformer::DummyTransformer(const std::string& manager_host, const std::string& token)
: metricq::Transformer(token), signals_(io_service, SIGINT, SIGTERM)
{ // Register signal handlers so that the daemon may be shut down.
    signals_.async_wait([this](auto, auto signal) {
        if (!signal)
        {
            return;
        }
        Log::info() << "Caught signal " << signal << ". Shutdown.";
        close();
    });

    connect(manager_host);
}

DummyTransformer::~DummyTransformer()
{
}

void DummyTransformer::on_transformer_config(const metricq::json& config)
{
    for (const auto& metric : config["metrics"])
    {
        const auto& out_id = metric["out_id"].get<std::string>();
        const auto& in_id = metric["in_id"].get<std::string>();
        const auto& factor = metric["factor"].get<double>();

        Log::info() << "Transforming " << in_id << " to " << out_id << " with factor " << factor;

        metric_info[in_id] = { out_id, factor };
        (*this)[out_id];
    }
}

void DummyTransformer::on_transformer_ready()
{
    Log::info() << "DummyTransformer ready";
}

void DummyTransformer::on_data(const std::string& id, metricq::TimeValue tv)
{
    try
    {
        Log::trace() << "DummyTransformer::on_data(" << id << ")";
        const auto& info = metric_info.at(id);
        tv.value *= info.factor;
        send(info.out_id, tv);
    }
    catch (const std::out_of_range&)
    {
        Log::error() << "received unexpected metric id: " << id;
        throw;
    }
}
