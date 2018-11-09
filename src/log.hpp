// Copyright (c) 2018, ZIH,
// Technische Universitaet Dresden,
// Federal Republic of Germany
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of metricq nor the names of its contributors
//       may be used to endorse or promote products derived from this software
//       without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#pragma once

#include <metricq/logger.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <memory>

namespace metricq
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
