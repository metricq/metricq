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

#include <metricq/chrono.hpp>
#include <metricq/sink.hpp>
#include <metricq/timer.hpp>

#include <asio/signal_set.hpp>

#include <atomic>

class DummySink : public metricq::Sink
{
public:
    DummySink(const std::string& manager_host, const std::string& token,
              const std::vector<std::string>& metrics, metricq::Duration timeout,
              std::size_t expected_chunk_count);

protected:
    void on_error(const std::string& message) override;
    void on_closed() override;

private:
    using metricq::Sink::on_data;

    void on_connected() override;

    void on_data_channel_ready() override;
    void on_data(const AMQP::Message& message, uint64_t delivery_tag, bool redelivered) override;
    void on_data(const std::string& id, metricq::TimeValue tv) override;

    asio::signal_set signals_;

    std::vector<std::string> metrics_;

public:
    metricq::TimePoint first_metric_time;
    std::size_t message_count = 0;

private:
    metricq::Duration timeout_;
    std::size_t expected_chunk_count_;

    std::size_t message_count_last_step_ = 0;
    std::size_t chunk_count_ = 0;
    metricq::TimePoint start_time_;
    metricq::TimePoint step_time_;

    metricq::Timer timer_;
    metricq::Timer timeout_timer_;

    std::atomic<bool> stop_requested_ = false;
};
