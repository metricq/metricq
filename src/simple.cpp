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
#include <metricq/simple.hpp>

#include <metricq/drain.hpp>
#include <metricq/rpc_request.hpp>
#include <metricq/simple_drain.hpp>
#include <metricq/subscriber.hpp>

#include <chrono>

namespace metricq
{
std::string subscribe(const std::string& url, const std::string& token,
                      const std::vector<std::string>& metrics, Duration timeout)
{
    Subscriber subscriber(token, timeout);

    subscriber.add(metrics);
    subscriber.connect(url);
    subscriber.main_loop();
    return subscriber.queue();
}

std::string subscribe(const std::string& url, const std::string& token, const std::string& metric,
                      Duration timeout)
{
    return subscribe(url, token, std::vector<std::string>{ metric }, timeout);
}

std::unordered_map<std::string, std::vector<TimeValue>>
drain(const std::string& url, const std::string& token, const std::vector<std::string>& metrics,
      const std::string& queue)
{
    SimpleDrain drain(token, queue);
    drain.add(metrics);
    drain.connect(url);
    drain.main_loop();
    return std::move(drain.get());
}

std::vector<TimeValue> drain(const std::string& url, const std::string& token,
                             const std::string& metric, const std::string& queue)
{
    SimpleDrain drain(token, queue);
    drain.add(metric);
    drain.connect(url);
    drain.main_loop();
    return std::move(drain.at(metric));
}

std::vector<std::string> get_metrics(const std::string& url, const std::string& token,
                                     const std::string& selector)
{
    json payload = json::object();
    payload["format"] = "array";
    if (!selector.empty())
    {
        payload["selector"] = selector;
    }
    RpcRequest request(token, "history.get_metrics", payload);
    request.connect(url);
    request.main_loop();
    std::vector<std::string> response;
    for (const auto& metric : request.response().at("metrics"))
    {
        response.push_back(metric.get<std::string>());
    }
    return response;
}

std::unordered_map<std::string, Metadata>
get_metadata_(const std::string& url, const std::string& token, const json& selector)
{
    json payload = json::object();
    payload["format"] = "object";
    if (!selector.empty())
    {
        payload["selector"] = selector;
    }
    RpcRequest request(token, "get_metrics", payload);
    request.connect(url);
    request.main_loop();
    std::unordered_map<std::string, Metadata> response;
    const auto& metrics_metadata = request.response().at("metrics");
    for (auto it = metrics_metadata.begin(); it != metrics_metadata.end(); ++it)
    {
        response.emplace(it.key(), it.value());
    }
    return response;
}

std::unordered_map<std::string, Metadata> get_metadata(const std::string& url,
                                                       const std::string& token)
{
    // manager will ignore a Null selector
    return get_metadata_(url, token, nullptr);
}

std::unordered_map<std::string, Metadata>
get_metadata(const std::string& url, const std::string& token, const std::string& selector)
{
    return get_metadata_(url, token, selector);
}

std::unordered_map<std::string, Metadata> get_metadata(const std::string& url,
                                                       const std::string& token,
                                                       const std::vector<std::string>& selector)
{
    return get_metadata_(url, token, selector);
}
} // namespace metricq
