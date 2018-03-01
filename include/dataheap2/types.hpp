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

    operator DataChunk() const
    {
        DataChunk dc;
        dc.add_time_delta(time.time_since_epoch().count());
        dc.add_value(value);
        return dc;
    }
};

template <typename T>
void data_chunk_foreach(const DataChunk& dc, T cb)
{
    int64_t timestamp = 0;
    auto value_iter = dc.value().begin();
    for (const auto& time_delta : dc.time_delta())
    {
        timestamp += time_delta;
        cb(dataheap2::TimeValue(dataheap2::TimePoint(dataheap2::Duration(timestamp)), *value_iter));
        value_iter++;
    }
}

class DataChunkIter
{
public:
    DataChunkIter(
        const DataChunk& dc,
        google::protobuf::RepeatedField<const google::protobuf::int64>::iterator iter_time,
        google::protobuf::RepeatedField<const double>::iterator iter_value)
    : dc(dc), iter_time(iter_time), iter_value(iter_value)
    {
    }

    dataheap2::TimeValue operator*() const
    {
        return { dataheap2::TimePoint(dataheap2::Duration(timestamp + *iter_time)),
                 *iter_value };
    }

    DataChunkIter& operator++()
    {
        timestamp += *iter_time;
        iter_time++;
        iter_value++;
        return *this;
    }

    bool operator!=(const DataChunkIter& other)
    {
        return iter_time != other.iter_time;
    }

private:
    const DataChunk& dc;
    google::protobuf::RepeatedField<const google::protobuf::int64>::iterator iter_time;
    google::protobuf::RepeatedField<const double>::iterator iter_value;
    int64_t timestamp = 0;
};

inline DataChunkIter begin(const DataChunk& dc)
{
    return { dc, dc.time_delta().begin(), dc.value().begin() };
}

inline DataChunkIter end(const DataChunk& dc)
{
    return { dc, dc.time_delta().end(), dc.value().end() };
}
}
