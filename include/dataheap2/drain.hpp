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
    virtual ~Drain() = 0;

    void add(const std::string &metric)
    {
        metrics_.emplace_back(metric);
    }
    template <typename T>
    void add(const T& metrics)
    {
        for (const auto& metric : metrics)
        {
            add(metric);
        }
    }

protected:
    void setup_complete() override;

private:
    void end();
    void unsubscribe_complete(const json& response);

protected:
    std::vector<std::string> metrics_;
};
}
