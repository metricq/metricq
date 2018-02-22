#pragma once

#include <dataheap2/chrono.hpp>
#include <protobufmessages/datapoint.pb.h>

namespace dataheap2
{
using Value = double;

struct TimeValue
{
    TimeValue() = default;

    constexpr TimeValue(TimePoint t, Value v) : time(t), value(v){};
    TimePoint time;
    Value value;

    operator DataPoint() const {
        DataPoint dp;
        dp.set_timestamp(time.time_since_epoch().count());
        dp.set_value(value);
        return dp;
    }
};
}
