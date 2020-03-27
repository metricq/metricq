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
#pragma once
#include <metricq/chrono.hpp>
#include <metricq/data_client.hpp>
#include <metricq/datachunk.pb.h>
#include <metricq/json_fwd.hpp>
#include <metricq/metric.hpp>
#include <metricq/types.hpp>

#include <amqpcpp.h>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace ev
{
class timer;
}

namespace metricq
{

class Source : public DataClient
{
public:
    using Metric = metricq::Metric<Source>;

    Source(const std::string& token);

    void send(const std::string& id, TimeValue tv);
    void send(const std::string& id, const DataChunk& dc);

    Metric& operator[](const std::string& id)
    {
        auto ret = metrics_.try_emplace(id, id, *this);
        return ret.first->second;
    }

protected:
    void on_connected() override;
    void on_data_channel_ready() override;

protected:
    virtual void on_source_config(const json& config) = 0;
    virtual void on_source_ready() = 0;

    void declare_metrics();
    void clear_metrics();

private:
    void on_register_response(const json& response);

protected:
    std::string data_exchange_;
    std::unordered_map<std::string, Metric> metrics_;
};
} // namespace metricq
