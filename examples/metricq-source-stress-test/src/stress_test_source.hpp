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
#pragma once

#include <metricq/source.hpp>
#include <metricq/timer.hpp>

#include <asio/signal_set.hpp>

#include <atomic>
#include <limits>
#include <random>
#include <system_error>

class StressTestSource : public metricq::Source
{
public:
    StressTestSource(const std::string& manager_host, const std::string& token);
    ~StressTestSource();

    void on_error(const std::string& message) override;
    void on_closed() override;

private:
    void on_source_config(const metricq::json& config) override;
    void on_source_ready() override;

private:
    asio::signal_set signals_;

    uint64_t chunk_size_;
    // per metric
    uint64_t remaining_batches_ = std::numeric_limits<uint64_t>::max();

public:
    uint64_t total_values = 0;
    metricq::TimePoint first_time_;

private:
    metricq::Timer timer_;
    std::atomic<bool> stop_requested_ = false;
    bool running_ = false;
    std::chrono::nanoseconds interval_;

    metricq::TimePoint previous_time_;

    std::vector<std::string> metrics_;

    std::vector<metricq::Value> fake_values_;
    std::vector<std::vector<metricq::Value>::iterator> fake_value_iter_;

    metricq::Timer::TimerResult timeout_cb(std::error_code);
};
