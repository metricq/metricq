#include <dataheap2/simple.hpp>

#include <dataheap2/drain.hpp>
#include <dataheap2/simple_drain.hpp>
#include <dataheap2/subscriber.hpp>

namespace dataheap2
{
std::string subscribe(const std::string& url, const std::string& token,
                      const std::vector<std::string>& metrics)
{
    Subscriber subscriber(token);

    subscriber.add(metrics);
    subscriber.connect(url);
    subscriber.main_loop();
    return subscriber.queue();
}

std::string subscribe(const std::string& url, const std::string& token, const std::string& metric)
{
    return subscribe(url, token, { metric });
}

std::unordered_map<std::string, std::vector<TimeValue>>
drain(const std::string& url, const std::string& token, const std::vector<std::string>& metrics,
      const std::string& queue)
{
    SimpleDrain drain(token, queue);
    drain.add(metrics);
    drain.connect(url);
    drain.main_loop();
    return drain.get();
}

std::vector<TimeValue> drain(const std::string& url, const std::string& token,
                             const std::string& metric, const std::string& queue)
{
    SimpleDrain drain(token, queue);
    drain.add(metric);
    drain.connect(url);
    drain.main_loop();
    return drain.at(metric);
}
}
