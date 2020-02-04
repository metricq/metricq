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
#pragma once

#include <metricq/chrono.hpp>
#include <metricq/datachunk.pb.h>
#include <metricq/history.pb.h>

namespace metricq
{
using Value = double;

struct TimeValue
{
    TimeValue() = default;

    constexpr TimeValue(TimePoint t, Value v) : time(t), value(v)
    {
    }

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

struct Aggregate
{
    Aggregate() = default;

    constexpr Aggregate(TimePoint t, Value min, Value max, Value sum, uint64_t count,
                        Value integral, Duration active_time)
    : time(t), min(min), max(max), sum(sum), count(count), integral(integral),
      active_time(active_time)
    {
    }

    Value mean_sum() const
    {
        return sum / count;
    }

    Value mean_integral() const
    {
        return integral / active_time.count();
    }

    Value mean() const
    {
        if (active_time.count())
        {
            return mean_integral();
        }
        return mean_sum();
    }

    TimePoint time;
    Value min;
    Value max;
    Value sum;
    uint64_t count;
    Value integral;
    Duration active_time;
};

struct [[deprecated]] TimeValueAggregate
{
    TimeValueAggregate() = default;

    constexpr TimeValueAggregate(TimePoint t, Value min, Value max, Value avg)
    : time(t), min(min), max(max), avg(avg)
    {
    }

    TimePoint time;
    Value min;
    Value max;
    Value avg;
};

template <typename T>
void data_chunk_foreach(const DataChunk& dc, T cb)
{
    int64_t timestamp = 0;
    auto value_iter = dc.value().begin();
    for (const auto& time_delta : dc.time_delta())
    {
        timestamp += time_delta;
        cb(metricq::TimeValue(metricq::TimePoint(metricq::Duration(timestamp)), *value_iter));
        value_iter++;
    }
}

class DataChunkIter
{
public:
    DataChunkIter(
        google::protobuf::RepeatedField<const google::protobuf::int64>::iterator iter_time,
        google::protobuf::RepeatedField<const double>::iterator iter_value)
    : iter_time(iter_time), iter_value(iter_value)
    {
    }

    metricq::TimeValue operator*() const
    {
        return { metricq::TimePoint(metricq::Duration(timestamp + *iter_time)), *iter_value };
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
    google::protobuf::RepeatedField<const google::protobuf::int64>::iterator iter_time;
    google::protobuf::RepeatedField<const double>::iterator iter_value;
    int64_t timestamp = 0;
};

inline DataChunkIter begin(const DataChunk& dc)
{
    return { dc.time_delta().begin(), dc.value().begin() };
}

inline DataChunkIter end(const DataChunk& dc)
{
    return { dc.time_delta().end(), dc.value().end() };
}

// Note for the remainder of this file:
// We provide a deprecated interface to the deprecated protobuf fields.
// As we are aware of that and marked the interface itself as deprecated, we will ignore the
// deprecation of the protobuf here

class [[deprecated]] HistoryRepsonseIter
{
public:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
    HistoryRepsonseIter(
        google::protobuf::RepeatedField<const google::protobuf::int64>::iterator iter_time,
        google::protobuf::RepeatedField<const double>::iterator iter_value_min,
        google::protobuf::RepeatedField<const double>::iterator iter_value_max,
        google::protobuf::RepeatedField<const double>::iterator iter_value_avg)
    : iter_time(iter_time), iter_value_min(iter_value_min), iter_value_max(iter_value_max),
      iter_value_avg(iter_value_avg)
    {
    }

    metricq::TimeValueAggregate operator*() const
    {
        return { metricq::TimePoint(metricq::Duration(timestamp + *iter_time)), *iter_value_min,
                 *iter_value_max, *iter_value_avg };
    }
#pragma GCC diagnostic pop

    HistoryRepsonseIter& operator++()
    {
        timestamp += *iter_time;
        iter_time++;
        iter_value_min++;
        iter_value_max++;
        iter_value_avg++;
        return *this;
    }

    bool operator!=(const HistoryRepsonseIter& other)
    {
        return iter_time != other.iter_time;
    }

private:
    google::protobuf::RepeatedField<const google::protobuf::int64>::iterator iter_time;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
    google::protobuf::RepeatedField<const double>::iterator iter_value_min;
    google::protobuf::RepeatedField<const double>::iterator iter_value_max;
    google::protobuf::RepeatedField<const double>::iterator iter_value_avg;
#pragma GCC diagnostic pop

    int64_t timestamp = 0;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
[[deprecated]] inline HistoryRepsonseIter begin(const HistoryResponse& hr)
{
    return { hr.time_delta().begin(), hr.value_min().begin(), hr.value_max().begin(),
             hr.value_avg().begin() };
}

[[deprecated]] inline HistoryRepsonseIter end(const HistoryResponse& hr)
{
    return { hr.time_delta().end(), hr.value_min().end(), hr.value_max().end(),
             hr.value_avg().end() };
}
#pragma GCC diagnostic pop

class HistoryResponseValueIterator
{
public:
    HistoryResponseValueIterator(
        google::protobuf::RepeatedField<const google::protobuf::int64>::iterator iter_time,
        google::protobuf::RepeatedField<const double>::iterator iter_value)
    : iter_time(iter_time), iter_value(iter_value)
    {
    }

    metricq::TimeValue operator*() const
    {
        return {
            metricq::TimePoint(metricq::Duration(timestamp + *iter_time)),
            *iter_value,
        };
    }

    HistoryResponseValueIterator& operator++()
    {
        timestamp += *iter_time;
        iter_time++;
        iter_value++;
        return *this;
    }

    bool operator!=(const HistoryResponseValueIterator& other)
    {
        return iter_time != other.iter_time;
    }

private:
    google::protobuf::RepeatedField<const google::protobuf::int64>::iterator iter_time;
    google::protobuf::RepeatedField<const double>::iterator iter_value;

    int64_t timestamp = 0;
};

class HistoryResponseAggregateIterator
{
public:
    HistoryResponseAggregateIterator(
        google::protobuf::RepeatedField<const google::protobuf::int64>::iterator iter_time,
        google::protobuf::RepeatedPtrField<HistoryResponse::Aggregate>::const_iterator
            iter_aggregate)
    : iter_time(iter_time), iter_aggregate(iter_aggregate)
    {
    }

    metricq::Aggregate operator*() const
    {
        return { metricq::TimePoint(metricq::Duration(timestamp + *iter_time)),
                 iter_aggregate->minimum(),
                 iter_aggregate->maximum(),
                 iter_aggregate->sum(),
                 iter_aggregate->count(),
                 iter_aggregate->integral(),
                 Duration(iter_aggregate->active_time()) };
    }

    HistoryResponseAggregateIterator& operator++()
    {
        timestamp += *iter_time;
        iter_time++;
        iter_aggregate++;
        return *this;
    }

    bool operator!=(const HistoryResponseAggregateIterator& other)
    {
        return iter_time != other.iter_time;
    }

private:
    google::protobuf::RepeatedField<const google::protobuf::int64>::iterator iter_time;
    google::protobuf::RepeatedPtrField<HistoryResponse::Aggregate>::const_iterator iter_aggregate;

    int64_t timestamp = 0;
};

class HistoryResponseValueView : public HistoryResponse
{
};

inline HistoryResponseValueIterator begin(const HistoryResponseValueView& hr)
{
    return { hr.time_delta().begin(), hr.value().begin() };
}

inline HistoryResponseValueIterator end(const HistoryResponseValueView& hr)
{
    return { hr.time_delta().end(), hr.value().end() };
}

class HistoryResponseAggregateView : public HistoryResponse
{
};

inline HistoryResponseAggregateIterator begin(const HistoryResponseAggregateView& hr)
{
    return { hr.time_delta().begin(), hr.aggregate().begin() };
}

inline HistoryResponseAggregateIterator end(const HistoryResponseAggregateView& hr)
{
    return { hr.time_delta().end(), hr.aggregate().end() };
}

} // namespace metricq
