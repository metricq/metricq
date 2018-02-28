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
        dc.set_timestamp_offset(time.time_since_epoch().count());
        dc.set_value(value);
        return dc;
    }
};

template <typename T>
void data_chunk_foreach(const DataChunk& dc, T cb)
{
    if (dc.has_value())
    {
        cb(dataheap2::TimeValue(dataheap2::TimePoint(dataheap2::Duration(dc.timestamp_offset())),
                                dc.value()));
        return;
    }

    auto offset = dc.timestamp_offset();
    for (const auto& data_point : dc.data())
    {
        offset += data_point.timestamp();
        cb(dataheap2::TimeValue(dataheap2::TimePoint(dataheap2::Duration(offset)),
                                data_point.value()));
    }
}

class DataChunkIter
{
public:
    DataChunkIter(const DataChunk& dc,
                  google::protobuf::internal::RepeatedPtrIterator<const DataPoint> iter,
                  bool end = false)
    : dc(dc), iter(iter), end(end)
    {
    }

    dataheap2::TimeValue operator*() const
    {
        if (dc.has_value())
        {
            return { dataheap2::TimePoint(dataheap2::Duration(dc.timestamp_offset())), dc.value() };
        }
        else
        {
            return { dataheap2::TimePoint(
                         dataheap2::Duration(dc.timestamp_offset() + iter->timestamp())),
                     iter->value() };
        }
    }

    DataChunkIter& operator++()
    {
        if (dc.has_value())
        {
            assert(!end);
            end = true;
        }
        else
        {
            iter++;
        }
        return *this;
    }

    bool operator!=(const DataChunkIter& other)
    {
        if (dc.has_value())
        {
            return end != other.end;
        }
        else
        {
            return iter != other.iter;
        }
    }

private:
    const DataChunk& dc;
    google::protobuf::internal::RepeatedPtrIterator<const DataPoint> iter;
    bool end = false;
};

DataChunkIter begin(const DataChunk& dc)
{
    return DataChunkIter(dc, dc.data().begin(), false);
}

DataChunkIter end(const DataChunk& dc)
{
    return { dc, dc.data().end(), true };
}
}

