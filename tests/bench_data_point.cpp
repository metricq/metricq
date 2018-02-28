#include "bench_data_common.hpp"

#include <protobufmessages/datachunk.pb.h>
#include <protobufmessages/datapoint.pb.h>

#include <dataheap2/types.hpp>

#include <benchmark/benchmark.h>

dataheap2::DataChunk generate(benchmark::State &)
{
    dataheap2::DataChunk data_chunk;
    data_chunk.set_timestamp_offset(1519829070761463639ll);
    data_chunk.set_value(1.);
    return data_chunk;
}

static void BM_manual(benchmark::State& state)
{
    dataheap2::DataChunk data_chunk = generate(state);
    for (auto _ : state)
    {
        consume_manual(data_chunk);
    }
    benchmark::DoNotOptimize(consume_sum);
    state.SetBytesProcessed(int64_t(state.iterations()));
}
BENCHMARK(BM_manual);

static void BM_foreach(benchmark::State& state)
{
    dataheap2::DataChunk data_chunk = generate(state);
    for (auto _ : state)
    {
        consume_foreach(data_chunk);
    }
    benchmark::DoNotOptimize(consume_sum);
    state.SetBytesProcessed(int64_t(state.iterations()));
}
BENCHMARK(BM_foreach);

static void BM_for(benchmark::State& state)
{
    dataheap2::DataChunk data_chunk = generate(state);
    for (auto _ : state)
    {
        consume_for(data_chunk);
    }
    benchmark::DoNotOptimize(consume_sum);
    state.SetBytesProcessed(int64_t(state.iterations()));
}
BENCHMARK(BM_for);

BENCHMARK_MAIN();
