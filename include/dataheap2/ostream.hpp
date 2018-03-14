#pragma once

#include <dataheap2/types.hpp>

#include <ostream>

namespace dataheap2
{
std::ostream& operator<<(std::ostream& os, Duration duration)
{
    os << duration.count();
    return os;
}

std::ostream& operator<<(std::ostream& os, TimePoint tp)
{
    os << tp.time_since_epoch();
    return os;
}

std::ostream& operator<<(std::ostream& os, TimeValue tv)
{
    os << tv.time << " " << tv.value;
    return os;
}
}
