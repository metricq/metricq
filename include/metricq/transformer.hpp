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

#include <metricq/json_fwd.hpp>
#include <metricq/metric.hpp>
#include <metricq/sink.hpp>

#include <unordered_map>
#include <vector>

namespace metricq
{
class Transformer : public Sink
{
public:
    using Metric = metricq::Metric<Transformer>;

    Transformer(const std::string& token);

    void send(const std::string& id, TimeValue tv);
    void send(const std::string& id, const DataChunk& dc);

    Metric& operator[](const std::string& id)
    {
        auto ret = output_metrics_.try_emplace(id, id, *this);
        return ret.first->second;
    }

protected:
    void on_connected() override;
    /**
     * Implementations of this function must add all required input_metrics
     */
    virtual void on_transformer_config(const json& config) = 0;
    virtual void on_transformer_ready() = 0;

    void declare_metrics();

private:
    // Subscribe to all metrics listed in input_metrics
    void subscribe_metrics();
    void on_register_response(const json& response);

private:
    std::string data_exchange_;
    std::unordered_map<std::string, Metric> output_metrics_;

protected:
    std::vector<std::string> input_metrics;
};
} // namespace metricq
