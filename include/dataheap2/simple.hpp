#pragma once

#include <dataheap2/types.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace dataheap2
{
std::string subscribe(const std::string& url, const std::string& token,
                      const std::vector<std::string>& metrics, std::chrono::seconds timeout);
std::string subscribe(const std::string& url, const std::string& token, const std::string& metric,
                      std::chrono::seconds timeout);

std::unordered_map<std::string, std::vector<TimeValue>>
drain(const std::string& url, const std::string& token, const std::vector<std::string>& metrics,
      const std::string& queue);
std::vector<TimeValue> drain(const std::string& url, const std::string& token,
                             const std::string& metric, const std::string& queue);
} // namespace dataheap2
