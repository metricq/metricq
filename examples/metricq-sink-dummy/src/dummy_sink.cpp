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
#include "dummy_sink.hpp"

#include <metricq/logger/nitro.hpp>

#include <metricq/ostream.hpp>
#include <metricq/types.hpp>

#include <nlohmann/json_fwd.hpp>

using json = nlohmann::json;

#include <cmath>

using Log = metricq::logger::nitro::Log;

DummySink::DummySink(const std::string& manager_host, const std::string& token,
                     const std::vector<std::string>& metrics)
: metricq::Sink(token, true), signals_(io_service, SIGINT, SIGTERM), metrics_(metrics)
{
    connect(manager_host);

    // Register signal handlers so that the daemon may be shut down.
    signals_.async_wait([this](auto, auto signal) {
        if (!signal)
        {
            return;
        }
        Log::info() << "Caught signal " << signal << ". Shutdown.";
        rpc("sink.unsubscribe", [this](const auto&) { (void)this; },
            { { "dataQueue", data_queue_ }, { "metrics", metrics_ } });
    });
}

void DummySink::on_connected()
{
    this->subscribe(metrics_);
    start_time_ = metricq::Clock::now();
}

void DummySink::on_data_channel_ready()
{
    Log::debug() << "DummySink data channel is ready! Metric metadata:";
}

void DummySink::on_error(const std::string& message)
{
    Log::debug() << "DummySink::on_error() called";
    Log::error() << "Shit hits the fan: " << message;
    signals_.cancel();
}

void DummySink::on_closed()
{
    Log::debug() << "DummySink::on_closed() called";
    signals_.cancel();
}

void DummySink::on_data(const AMQP::Message& message, uint64_t delivery_tag, bool redelivered)
{
    if (message.typeName() == "end")
    {
        data_channel_->ack(delivery_tag);
        Log::info() << "received end message, requesting release and stop";
        // We used to close the data connection here, but this should not be necessary.
        // It will be closed implicitly from the response callback.
        rpc("sink.release", [this](const auto&) { close(); }, { { "dataQueue", data_queue_ } });
        return;
    }

    Sink::on_data(message, delivery_tag, redelivered);
}

void DummySink::on_data(const std::string&, metricq::TimeValue)
{
    if (message_count == 0)
    {
        Log::info() << "received first timevalue";
        first_metric_time = metricq::Clock::now();
    }
    message_count++;
}
