#pragma once

#include <dataheap2/drain.hpp>
#include <dataheap2/ostream.hpp>
#include <dataheap2/types.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace dataheap2
{
class SimpleDrain : public Drain
{
public:
    SimpleDrain(const std::string& token, const std::string& queue) : Drain(token, queue)
    {
    }

    void data_callback(const std::string& id, const dataheap2::DataChunk& chunk) override
    {
        auto& d = data_.at(id);
        for (const auto& tv : chunk)
        {
            d.emplace_back(tv);
        }
    }

    /**
     * warning this remove the entire map
     */
    std::unordered_map<std::string, std::vector<TimeValue>> get()
    {
        return std::move(data_);
    };

    /**
     * warning this remove the vector
     */
    std::vector<TimeValue> at(const std::string& metric)
    {
        return std::move(data_.at(metric));
    }

protected:
    void setup_complete() override
    {
        Drain::setup_complete();
        for (const auto& metric : metrics_)
        {
            data_[metric];
        }
    }

private:
    std::unordered_map<std::string, std::vector<TimeValue>> data_;
};
}
