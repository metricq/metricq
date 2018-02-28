#include "bench_data_common.hpp"

#include <protobufmessages/datachunk.pb.h>
#include <protobufmessages/datapoint.pb.h>

#include <dataheap2/types.hpp>

#include <benchmark/benchmark.h>


dataheap2::DataChunk generate(benchmark::State& state)
{
    dataheap2::DataChunk data_chunk;
    double value = 1.0;
    int64_t time = 1519832293179227888;
    data_chunk.set_timestamp_offset(time);
    for (int i = 0; i < state.range(0); i++)
    {
        auto data_point = data_chunk.add_data();
        data_point->set_timestamp(time);
        data_point->set_value(value + i);
    }
    return data_chunk;
}

static void BM_manual(benchmark::State& state)
{
    dataheap2::DataChunk data_chunk = generate(state);
    for (auto _ : state)
    {
        consume_manual(data_chunk);
    }
    // actually number of elements, not bytes:
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data_chunk.data_size()));
    benchmark::DoNotOptimize(consume_sum);
}
BENCHMARK(BM_manual)->Range(1, 1<<20);

static void BM_foreach(benchmark::State& state)
{
    dataheap2::DataChunk data_chunk = generate(state);
    for (auto _ : state)
    {
        consume_foreach(data_chunk);
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data_chunk.data_size()));
    benchmark::DoNotOptimize(consume_sum);
}
BENCHMARK(BM_foreach)->Range(1, 1<<20);

static void BM_for(benchmark::State& state)
{
    dataheap2::DataChunk data_chunk = generate(state);
    for (auto _ : state)
    {
        consume_for(data_chunk);
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data_chunk.data_size()));
    benchmark::DoNotOptimize(consume_sum);
}
BENCHMARK(BM_for)->Range(1, 1<<20);

BENCHMARK_MAIN();
