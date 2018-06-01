#pragma once

#include <chrono>
#include <string>

#include <cinttypes>

namespace dataheap2
{
using Duration = std::chrono::duration<int64_t, std::nano>;

struct Clock
{
    using duration = Duration;
    using rep = duration::rep;
    using period = duration::period;
    // If this ever breaks, it's all Mario's fault
    using time_point = std::chrono::time_point<std::chrono::system_clock, duration>;
    static const bool is_steady = true;
    static time_point now()
    {
        // TODO use clock_gettime and all the funny stuff
        return time_point(std::chrono::duration_cast<duration>(
            std::chrono::system_clock::now().time_since_epoch()));
    }
    static std::string format_iso(time_point tp);
    static std::string format(time_point tp, std::string fmt);
    static std::time_t to_time_t(time_point tp)
    {
        return std::chrono::system_clock::to_time_t(std::chrono::system_clock::time_point(
            std::chrono::duration_cast<std::chrono::system_clock::duration>(
                tp.time_since_epoch())));
    }
};

using TimePoint = Clock::time_point;

template <typename FromDuration, typename ToDuration = Duration>
constexpr ToDuration duration_cast(const FromDuration& dtn)
{
    return std::chrono::duration_cast<ToDuration>(dtn);
}
} // namespace dataheap2
