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
#include "stress_test_source.hpp"

#include <metricq/logger/nitro.hpp>

#include <metricq/chrono.hpp>
#include <metricq/types.hpp>

#include <cmath>

using Log = metricq::logger::nitro::Log;

StressTestSource::StressTestSource(const std::string& manager_host, const std::string& token,
                                   int interval_ms)
: metricq::Source(token), signals_(io_service, SIGINT, SIGTERM), interval_ms(interval_ms), t(0),
  timer_(io_service)
{
    Log::debug() << "StressTestSource::StressTestSource() called";

    // Register signal handlers so that the daemon may be shut down.
    signals_.async_wait([this](auto, auto signal) {
        if (!signal)
        {
            return;
        }
        Log::info() << "Caught signal " << signal << ". Shutdown.";
        if (timer_.running())
        {
            stop_requested_ = true;
        }
        else
        {
            Log::info() << "closing source";
            stop();
        }
    });

    connect(manager_host);
}

StressTestSource::~StressTestSource()
{
}

void StressTestSource::on_source_config(const nlohmann::json& config)
{
    Log::debug() << "StressTestSource::on_source_config() called";

    metric_ = config.at("metric");

    (*this)[metric_];
}

void StressTestSource::on_source_ready()
{
    Log::debug() << "StressTestSource::on_source_ready() called";
    (*this)[metric_].metadata.unit("kittens");
    (*this)[metric_].metadata["color"] = "pink";
    (*this)[metric_].metadata["paws"] = 4;

    auto current_time = metricq::Clock::now();

    timer_.start([this](auto err) { return this->timeout_cb(err); },
                 std::chrono::milliseconds(interval_ms));

    running_ = true;
}

void StressTestSource::on_error(const std::string& message)
{
    Log::debug() << "StressTestSource::on_error() called";
    Log::error() << "Shit hits the fan: " << message;
    signals_.cancel();
    timer_.cancel();
}

void StressTestSource::on_closed()
{
    Log::debug() << "StressTestSource::on_closed() called";
    signals_.cancel();
    timer_.cancel();
}

metricq::Timer::TimerResult StressTestSource::timeout_cb(std::error_code)
{
    if (stop_requested_)
    {
        Log::info() << "closing source and stopping metric timer";
        stop();
        return metricq::Timer::TimerResult::cancel;
    }
    Log::debug() << "sending metrics...";
    const auto r = 100000;
    auto& metric = (*this)[metric_];
    metric.chunk_size(0);
    for (int i = 0; i < r; i++)
    {
        double value = 2 * M_PI * (t + (double)i / r) / interval_ms;
        metric.send({ current_time, value });
        current_time +=
            std::chrono::duration_cast<metricq::Duration>(std::chrono::milliseconds(interval_ms)) /
            (r + 1);
    }
    metric.flush();
    t++;
    return metricq::Timer::TimerResult::repeat;
}
