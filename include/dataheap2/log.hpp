#pragma once

#include <memory>
#include <string>

namespace dataheap2
{
class Logger
{
public:
    virtual ~Logger() = 0;

    virtual void trace(const std::string&){};
    virtual void debug(const std::string&){};
    virtual void notice(const std::string&){};
    virtual void info(const std::string&){};
    virtual void warn(const std::string&){};
    virtual void error(const std::string&){};
    virtual void fatal(const std::string&){};
};

class LoggerSingleton
{
public:
    static std::unique_ptr<Logger>& get();
};

template <typename L, typename... Args>
inline void make_logger(Args&&... args)
{
    LoggerSingleton::get() = std::make_unique<L>(std::forward(args...));
}
}
