#include <dataheap2/subscriber.hpp>

#include <chrono>
#include <string>

namespace dataheap2
{

Subscriber::Subscriber(const std::string& token, std::chrono::seconds timeout, bool add_uuid)
: Connection(token, add_uuid), timeout_(timeout)
{
}

void Subscriber::add(const std::string& metric)
{
    metrics_.emplace_back(metric);
}

void Subscriber::setup_complete()
{
    rpc("subscribe",
        [this](const json& response) {
            queue_ = response["dataQueue"];
            stop();
        },
        { { "metrics", metrics_ }, { "expires", timeout_.count() } });
}
} // namespace dataheap2
