#include "log.hpp"

#include <iostream>
#include <memory>
#include <string>

namespace dataheap2
{
Logger::~Logger()
{
}

class DefaultLogger : public Logger
{
    void warn(const std::string& msg) override
    {
        std::cerr << "[dataheap2]" << msg << "\n";
    }

    void error(const std::string& msg) override
    {
        std::cerr << "[dataheap2]" << msg << "\n";
    }

    void fatal(const std::string& msg) override
    {
        std::cerr << "[dataheap2]" << msg << "\n";
    }
};

std::unique_ptr<Logger>& LoggerSingleton::get()
{
    static std::unique_ptr<Logger> logger = std::make_unique<DefaultLogger>();
    return logger;
}
}
