// Copyright (c) 2019, ZIH, Technische Universitaet Dresden, Federal Republic of Germany
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
#include "dummy_history.hpp"

#include <metricq/logger/nitro.hpp>

#include <metricq/ostream.hpp>
#include <metricq/types.hpp>

#include <chrono>

#include <cmath>

using Log = metricq::logger::nitro::Log;

DummyHistory::DummyHistory(const std::string& manager_host, const std::string& token,
                           const std::vector<std::string>& metrics)
: metricq::HistoryClient(token), metrics_(metrics), signals_(io_service, SIGINT, SIGTERM)
{
    Log::debug() << "DummyHistory::DummyHistory() called";

    // Register signal handlers so that the daemon may be shut down.
    signals_.async_wait([this](auto, auto signal) {
        if (!signal)
        {
            return;
        }
        Log::info() << "Caught signal " << signal << ". Shutdown.";
        this->stop();
    });

    connect(manager_host);
}

DummyHistory::~DummyHistory()
{
}

void DummyHistory::on_history_config(const metricq::json&)
{
    Log::debug() << "DummyHistory::on_history_config() called";
}

void DummyHistory::on_history_ready()
{
    Log::debug() << "DummyHistory::on_history_ready() called";

    auto now = metricq::Clock::now();

    // request last 100 seconds for each metric
    for (auto& metric : metrics_)
    {
        auto id = history_request(metric, now - metricq::Duration(100000000000), now,
                                  metricq::Duration(10000000000));

        Log::info() << "Created HistoryRequest: " << id;
    }
}

void DummyHistory::on_error(const std::string& message)
{
    Log::debug() << "DummyHistory::on_error() called";
    Log::error() << "Shit hits the fan: " << message;
    signals_.cancel();
}

void DummyHistory::on_closed()
{
    Log::debug() << "DummyHistory::on_closed() called";
    signals_.cancel();
}

void DummyHistory::on_history_response(const std::string& id,
                                       const metricq::HistoryResponseValueView& response)
{
    Log::debug() << "DummyHistory::on_history_response() called";

    Log::info() << "Got HistoryResponseValue for metric: " << response.metric() << "(" << id << ")";

    for (auto tv : response)
    {
        Log::info() << tv.time << ": " << tv.value;
    }

    io_service.post([this]() { this->stop(); });
}

void DummyHistory::on_history_response(const std::string& id,
                                       const metricq::HistoryResponseAggregateView& response)
{
    Log::debug() << "DummyHistory::on_history_response() called";

    Log::info() << "Got HistoryResponseAggregate for metric: " << response.metric() << "(" << id
                << ")";

    for (auto tva : response)
    {
        Log::info() << tva.time << ": " << tva.mean() << " (" << tva.min << " - " << tva.max
                    << ") @ " << tva.count;
    }

    io_service.post([this]() { this->stop(); });
}
