#pragma once

#include <exception>
#include <string>

namespace metricq
{
class Exception : public std::exception
{
};

class ConnectionClosed : public Exception
{
public:
    ConnectionClosed(const std::string& reason = "connection closed") : reason_(reason)
    {
    }

    const char* what() const noexcept override
    {
        return reason_.c_str();
    }

private:
    std::string reason_;
};
} // namespace metricq