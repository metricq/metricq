#pragma once

#include <metricq/drain.hpp>

#include <string>
#include <unordered_map>

class CountDrain : public metricq::Drain
{
public:
    CountDrain(const std::string& token, const std::string& queue) : Drain(token, queue)
    {
    }

    using Drain::on_data;

    void on_data(const std::string& id, const metricq::DataChunk& chunk) override
    {
        counts[id] += chunk.value_size();
    }

    std::unordered_map<std::string, int64_t> counts;
};