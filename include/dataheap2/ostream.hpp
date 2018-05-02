#pragma once

#include <dataheap2/types.hpp>

#include <ostream>

// USE THIS HEADER WITH CARE
// Don't use if you have other operator<< on std::chrono::duration defined

// Unfortunately duration is actually a (using) std type, so we can't
// define this in datheap2
namespace std
{
inline std::ostream& operator<<(std::ostream& os, dataheap2::Duration duration)
{
    os << duration.count();
    return os;
}
} // namespace std

namespace dataheap2
{
inline std::ostream& operator<<(std::ostream& os, TimePoint tp)
{
    os << tp.time_since_epoch();
    return os;
}

inline std::ostream& operator<<(std::ostream& os, TimeValue tv)
{
    os << tv.time << " " << tv.value;
    return os;
}
} // namespace dataheap2
