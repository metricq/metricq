#pragma once

#include <protobufmessages/datachunk.pb.h>
#include <protobufmessages/datapoint.pb.h>

#include <dataheap2/types.hpp>

double consume_sum = 0;
void consume(dataheap2::TimeValue tv)
{
    consume_sum += tv.value;
}

void consume_manual(const dataheap2::DataChunk& data_chunk)
{
    if (data_chunk.has_value())
    {
        consume({ dataheap2::TimePoint(dataheap2::Duration(data_chunk.timestamp_offset())),
                  data_chunk.value() });
        return;
    }

    auto offset = data_chunk.timestamp_offset();
    for (const auto& data_point : data_chunk.data())
    {
        offset += data_point.timestamp();
        consume({ dataheap2::TimePoint(dataheap2::Duration(offset)), data_point.value() });
    }
}

void consume_for(const dataheap2::DataChunk& data_chunk)
{
    for (const auto& elem : data_chunk)
    {
        consume(elem);
    }
}

void consume_foreach(const dataheap2::DataChunk& data_chunk)
{
    data_chunk_foreach(data_chunk, [](dataheap2::TimeValue tv) { consume(tv); });
}
