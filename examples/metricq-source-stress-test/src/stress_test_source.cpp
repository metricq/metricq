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
#include <metricq/ostream.hpp>
#include <metricq/types.hpp>

#include <chrono>
#include <cmath>
#include <exception>
#include <fstream>

#include "pcg_random.hpp"

using Log = metricq::logger::nitro::Log;

StressTestSource::StressTestSource(const std::string& manager_host, const std::string& token)
: metricq::Source(token), signals_(io_service, SIGINT, SIGTERM), timer_(io_service)
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

void StressTestSource::on_source_config(const metricq::json& config)
{
    Log::debug() << "StressTestSource::on_source_config() called";

    std::string prefix = config.at("prefix");
    int num_metrics = config.at("num_metrics");
    for (int i = 0; i < num_metrics; i++)
    {
        auto name = prefix + "." + std::to_string(i);
        metrics_.emplace_back(name);
        (*this)[name];
        (*this)[name].metadata.unit("kittens");
        (*this)[name].metadata["color"] = "pink";
        (*this)[name].metadata["sibling"] = i;
    }

    double rate = config.at("rate");
    chunk_size_ = config.at("batch_size");
    interval_ = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::nanoseconds(1000000000ll) / (rate / chunk_size_));

    Log::info() << "Rate: " << rate << " chunk size: " << chunk_size_
                << " timer interval: " << interval_.count() << "ns";

    if (config.count("duration"))
    {
        auto duration = metricq::Duration(config.at("duration")); // of course in nanoseconds
        remaining_batches_ = duration / interval_;
        Log::info() << "limiting execution to " << duration << " ns, " << remaining_batches_
                    << " batches.";
    }

    pcg64 random;

    if (config.count("random_seed"))
    {
        int seed = config.count("random_seed");
        random.seed(seed);
    }

    if (config.count("value_file")) // fake value delivery!
    {
        std::ifstream file;
        file.exceptions(std::ifstream::badbit);
        std::string filename = config.at("value_file");
        file.open(filename);
        file.seekg(0, std::fstream::end);
        auto size_bytes = file.tellg();
        if (size_bytes == 0 || size_bytes % sizeof(metricq::Value))
        {
            throw std::runtime_error("file size empty or not a multiple of sizeof metricq::Value");
        }
        Log::info() << "Loading: " << size_bytes / sizeof(metricq::Value) << " values from "
                    << filename;
        fake_values_.resize(size_bytes / sizeof(metricq::Value));
        file.seekg(0);
        file.read(reinterpret_cast<char*>(fake_values_.data()), size_bytes);
    }
    else // prepare some fake values ourselves
    {
        Log::info() << "Generating 4096 fake values mean 100, stddev 10";
        std::normal_distribution<metricq::Value> distribution(100, 10);
        fake_values_.reserve(4096);
        for (int i = 0; i < 4096; i++)
        {
            fake_values_.push_back(distribution(random));
        }
    }

    // let all threads start at random positions
    std::uniform_int_distribution<size_t> distribution(0, fake_values_.size() - 1);
    for (int i = 0; i < num_metrics; i++)
    {
        fake_value_iter_.push_back(fake_values_.begin() + distribution(random));
    }
}

void StressTestSource::on_source_ready()
{
    Log::debug() << "StressTestSource::on_source_ready() called";

    previous_time_ = metricq::Clock::now();

    timer_.start([this](auto err) { return this->timeout_cb(err); }, interval_);

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
    auto current_time = metricq::Clock::now();

    if (first_time_.time_since_epoch().count() == 0)
    {
        Log::info() << "first timer";
        first_time_ = current_time;
    }

    // Use double to get good precision with very short intervals
    auto actual_interval =
        std::chrono::duration_cast<std::chrono::duration<double>>(current_time - previous_time_);

    auto fake_value_iter_iter = fake_value_iter_.begin(); // HAHAHA gotcha
    for (const auto& name : metrics_)
    {
        auto& metric = (*this)[name];
        metric.chunk_size(0);
        for (uint64_t chunk_index = 0; chunk_index < chunk_size_; chunk_index++)
        {
            auto time = previous_time_ + metricq::duration_cast(chunk_index * actual_interval);
            assert(time < current_time);
            auto value = **fake_value_iter_iter;
            if (++(*fake_value_iter_iter) == fake_values_.end())
            {
                *fake_value_iter_iter = fake_values_.begin();
            }

            metric.send({ time, value });
            total_values++;
        }
        metric.flush();
        fake_value_iter_iter++;
    }

    if (--remaining_batches_ == 0)
    {
        Log::info() << "final batch completed";

        AMQP::Envelope envelope(nullptr, 0);
        envelope.setTypeName("end");
        data_channel_->publish(data_exchange_, this->metrics_.front(), envelope);

        stop();
        return metricq::Timer::TimerResult::cancel;
    }

    return metricq::Timer::TimerResult::repeat;
}
