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
        auto data_point = dc.add_data();
        data_point->set_time_delta(time.time_since_epoch().count());
        data_point->set_value(value);
        return dc;
    }
};

template <typename T>
void data_chunk_foreach(const DataChunk& dc, T cb)
{
    int64_t timestamp = 0;
    for (const auto& data_point : dc.data())
    {
        timestamp += data_point.time_delta();
        cb(dataheap2::TimeValue(dataheap2::TimePoint(dataheap2::Duration(timestamp)),
                                data_point.value()));
    }
}

class DataChunkIter
{
public:
    DataChunkIter(const DataChunk& dc,
                  google::protobuf::internal::RepeatedPtrIterator<const DataPoint> iter)
    : dc(dc), iter(iter)
    {
    }

    dataheap2::TimeValue operator*() const
    {
        return { dataheap2::TimePoint(dataheap2::Duration(timestamp + iter->time_delta())),
                 iter->value() };
    }

    DataChunkIter& operator++()
    {
        timestamp += iter->time_delta();
        iter++;
        return *this;
    }

    bool operator!=(const DataChunkIter& other)
    {
        return iter != other.iter;
    }

private:
    const DataChunk& dc;
    google::protobuf::internal::RepeatedPtrIterator<const DataPoint> iter;
    int64_t timestamp = 0;
};

inline DataChunkIter begin(const DataChunk& dc)
{
    return { dc, dc.data().begin() };
}

inline DataChunkIter end(const DataChunk& dc)
{
    return { dc, dc.data().end() };
}
}
