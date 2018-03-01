#pragma once

#include <protobufmessages/datachunk.pb.h>
#include <protobufmessages/datapoint.pb.h>

#include <dataheap2/types.hpp>

double consume_sum = 0;
void consume(dataheap2::TimeValue tv)
{
    consume_sum += tv.value;
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
