#pragma once

#include <metricq/datachunk.pb.h>

#include <metricq/types.hpp>

double consume_sum = 0;
void consume(metricq::TimeValue tv)
{
    consume_sum += tv.value;
}

void consume_for(const metricq::DataChunk& data_chunk)
{
    for (const auto& elem : data_chunk)
    {
        consume(elem);
    }
}

void consume_foreach(const metricq::DataChunk& data_chunk)
{
    data_chunk_foreach(data_chunk, [](metricq::TimeValue tv) { consume(tv); });
}
