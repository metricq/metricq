#pragma once

#include <dataheap2/log.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <memory>

namespace dataheap2
{
namespace log
{
    template <typename... Args>
    void trace(fmt::string_view format, const Args&... args)
    {
        LoggerSingleton::get()->trace(fmt::format(format, args...));
    }

    template <typename... Args>
    void debug(fmt::string_view format, const Args&... args)
    {
        LoggerSingleton::get()->debug(fmt::format(format, args...));
    }

    template <typename... Args>
    void notice(fmt::string_view format, const Args&... args)
    {
        LoggerSingleton::get()->notice(fmt::format(format, args...));
    }

    template <typename... Args>
    void info(fmt::string_view format, const Args&... args)
    {
        LoggerSingleton::get()->info(fmt::format(format, args...));
    }

    template <typename... Args>
    void warn(fmt::string_view format, const Args&... args)
    {
        LoggerSingleton::get()->warn(fmt::format(format, args...));
    }

    template <typename... Args>
    void error(fmt::string_view format, const Args&... args)
    {
        LoggerSingleton::get()->error(fmt::format(format, args...));
    }

    template <typename... Args>
    void fatal(fmt::string_view format, const Args&... args)
    {
        LoggerSingleton::get()->fatal(fmt::format(format, args...));
    }
}
}
