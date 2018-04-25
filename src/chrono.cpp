#include <dataheap2/chrono.hpp>

#include <iomanip>
#include <regex>
#include <sstream>
#include <string>

#include <ctime>

namespace dataheap2
{
std::string Clock::format(dataheap2::Clock::time_point tp, std::string fmt)
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

std::string Clock::format_iso(dataheap2::Clock::time_point tp)
{
    return format(tp, "%Y-%m-%dT%H:%M:%S.%f%z");
}
} // namespace dataheap2
