#pragma once

#include <dataheap2/connection.hpp>

#include <chrono>
#include <string>
#include <vector>

namespace dataheap2
{
class Subscriber : public Connection
{
public:
    explicit Subscriber(const std::string& token, std::chrono::seconds timeout, bool add_uuid
    = true);

    void add(const std::string& metric);
    template <typename T>
    void add(const T& metrics)
    {
        for (const auto& metric : metrics)
        {
            add(metric);
        }
    }
    const std::string& queue() const
    {
        return queue_;
    }

protected:
    void setup_complete() override;

protected:
    std::vector<std::string> metrics_;
    std::string queue_;
    std::chrono::seconds timeout_;
};
}
