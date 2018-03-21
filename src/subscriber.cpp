#include <dataheap2/subscriber.hpp>

#include <string>

namespace dataheap2
{

Subscriber::Subscriber(const std::string& token, bool add_uuid)
: Connection(token, add_uuid)
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
        { { "metrics", metrics_ } });
}
}
