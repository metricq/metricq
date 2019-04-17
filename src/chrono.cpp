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
#include <metricq/chrono.hpp>

#include <iomanip>
#include <regex>
#include <sstream>
#include <string>

#include <cassert>
#include <ctime>

namespace metricq
{
std::string Clock::format(metricq::Clock::time_point tp, std::string fmt)
{
    if (fmt.find("%f") != std::string::npos)
    {
        std::stringstream s;

        s << std::setfill('0') << std::setw(9)
          << (tp.time_since_epoch() % std::chrono::seconds(1)).count();

        fmt = std::regex_replace(fmt, std::regex("%f"), s.str());
    }
    // as fmt == "%p" could result in an empty string, we need to
    // force at least on character in the output format.
    // So if it fits, size will be at least one.
    fmt += '\a';
    std::string buffer;
    std::size_t size_in = std::max(std::size_t(200), fmt.size());
    std::size_t size;

    auto time = Clock::to_time_t(tp);
    std::tm tm_data;
    localtime_r(&time, &tm_data);

    do
    {
        size_in *= 1.6;
        buffer.resize(size_in);

        size = strftime(&buffer[0], buffer.size(), fmt.data(), &tm_data);
    } while (size == 0);

    // remove the trailing additional character
    buffer.resize(size - 1);

    return buffer;
}

std::string Clock::format_iso(metricq::Clock::time_point tp)
{
    return format(tp, "%Y-%m-%dT%H:%M:%S.%f%z");
}

Duration duration_parse(const std::string& str)
{
    // Note that regex_match only considers full matches, so no ^$ needed
    static std::regex number_and_unit("([+-]?\\d*[.,]?\\d+)\\s*([^\\d]*)");
    std::smatch match;
    if (!std::regex_match(str, match, number_and_unit))
    {
        throw std::invalid_argument("invalid duration string \"" + str +
                                    "\", not of form \"number unit\"");
    }
    assert(match.size() == 2);
    double value = std::stod(match[1]);
    std::string unit = match[2];
    // TODO C++20 use .starts_with
    // Note: we use duration<double> to avoid distinguishing between double and integral types
    // If you specify like 53 days as "4579200000000000 nanosecond"
    // Let the duration cast figure out the nifty details, hopefully correctly
    if (unit == "" or unit == "s" or unit == "seconds")
    {
        return duration_cast(std::chrono::duration<double>(value));
    }
    if (unit == "ms" or unit == "milliseconds")
    {
        return duration_cast(std::chrono::duration<double, std::milli>(value));
    }
    if (unit == "us" or unit == "microseconds" or unit == "μs")
    {
        return duration_cast(std::chrono::duration<double, std::micro>(value));
    }
    if (unit == "ns" or unit == "nanoseconds")
    {
        return duration_cast(std::chrono::duration<double, std::nano>(value));
    }
    if (unit == "min" or unit == "minutes")
    {
        return duration_cast(std::chrono::duration<double, std::ratio<60>>(value));
    }
    if (unit == "h" or unit == "hours")
    {
        return duration_cast(std::chrono::duration<double, std::ratio<3600>>(value));
    }
    throw std::invalid_argument("invalid duration unit \"" + unit + "\"");
}
} // namespace metricq
