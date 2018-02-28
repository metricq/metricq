#pragma once

#include <dataheap2/chrono.hpp>
#include <protobufmessages/datachunk.pb.h>

namespace dataheap2
{
using Value = double;

struct TimeValue
{
    TimeValue() = default;

    constexpr TimeValue(TimePoint t, Value v) : time(t), value(v){};
    TimePoint time;
    Value value;

    operator DataChunk() const {
        DataChunk dc;
        dc.set_timestamp_offset(time.time_since_epoch().count());
        dc.set_value(value);
        return dc;
    }
};
}
