#pragma once
#include <dataheap2/sink.hpp>

#include <nlohmann/json_fwd.hpp>

#include <string>
#include <vector>

namespace dataheap2
{
using json = nlohmann::json;

class Drain : public Sink
{
public:
    Drain(const std::string& token, const std::string& queue) : Sink(token, true)
    {
        data_queue_ = queue;
    }

    void add_metric(const std::string& metric)
    {
        metrics_.emplace_back(metric);
    }

    void setup_complete() override;

private:
    void end();
    void unsubscribe_complete(const json& response);

private:
    std::vector<std::string> metrics_;
};
}
