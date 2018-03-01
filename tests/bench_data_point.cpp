#include "bench_data_common.hpp"

#include <protobufmessages/datachunk.pb.h>
#include <protobufmessages/datapoint.pb.h>

#include <dataheap2/types.hpp>

#include <benchmark/benchmark.h>

void generate(dataheap2::DataChunk& data_chunk, benchmark::State&)
{
    data_chunk.set_timestamp_offset(1519832293179227888);
    data_chunk.set_value(1. / 3);
}

static void BM_generate(benchmark::State& state)
{
    dataheap2::DataChunk data_chunk;
    for (auto _ : state)
    {
        generate(data_chunk, state);
        benchmark::DoNotOptimize(data_chunk);
        data_chunk.clear_timestamp_offset();
        data_chunk.clear_value();
        data_chunk.clear_data();
    }
    state.SetBytesProcessed(int64_t(state.iterations()));
}
BENCHMARK(BM_generate);

static void BM_serialize(benchmark::State& state)
{
    dataheap2::DataChunk data_chunk;
    generate(data_chunk, state);
    std::string string;
    for (auto _ : state)
    {
        data_chunk.SerializeToString(&string);
        benchmark::DoNotOptimize(string);
    }
    state.SetBytesProcessed(int64_t(state.iterations()));
}
BENCHMARK(BM_serialize);

static void BM_parse(benchmark::State& state)
{
    dataheap2::DataChunk data_chunk;
    generate(data_chunk, state);
    std::string string;
    data_chunk.SerializeToString(&string);
    for (auto _ : state)
    {
        data_chunk.ParseFromString(string);
        benchmark::DoNotOptimize(data_chunk);
    }
    state.SetBytesProcessed(int64_t(state.iterations()));
}
BENCHMARK(BM_parse);

static void BM_manual(benchmark::State& state)
{
    dataheap2::DataChunk data_chunk;
    generate(data_chunk, state);
    for (auto _ : state)
    {
        consume_manual(data_chunk);
        benchmark::DoNotOptimize(consume_sum);
    }
    state.SetBytesProcessed(int64_t(state.iterations()));
}
BENCHMARK(BM_manual);

static void BM_foreach(benchmark::State& state)
{
    dataheap2::DataChunk data_chunk;
    generate(data_chunk, state);
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
    dataheap2::DataChunk data_chunk;
    generate(data_chunk, state);
    for (auto _ : state)
    {
        consume_for(data_chunk);
    }
    benchmark::DoNotOptimize(consume_sum);
    state.SetBytesProcessed(int64_t(state.iterations()));
}
BENCHMARK(BM_for);

BENCHMARK_MAIN()
