#include "bench_data_common.hpp"

#include <protobufmessages/datachunk.pb.h>
#include <protobufmessages/datapoint.pb.h>

#include <dataheap2/types.hpp>

#include <benchmark/benchmark.h>

void generate(dataheap2::DataChunk& data_chunk, benchmark::State& state)
{
    constexpr double value = 1.0 / 3;
    int64_t time = 1519832293179227888;

    for (int i = 0; i < state.range(0); i++)
    {
        auto data_point = data_chunk.add_data();
        if (i == 0)
        {
            data_point->set_time_delta(time);
        }
        else
        {
            data_point->set_time_delta(1000);
        }
        data_point->set_value(value + i);
    }
}

static void BM_generate(benchmark::State& state)
{
    dataheap2::DataChunk data_chunk;
    for (auto _ : state)
    {
        generate(data_chunk, state);
        benchmark::DoNotOptimize(data_chunk);
        data_chunk.clear_data();
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range(0)));
}
BENCHMARK(BM_generate)->Range(1, 1 << 20);

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
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data_chunk.data_size()));
}
BENCHMARK(BM_serialize)->Range(1, 1 << 20);

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
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data_chunk.data_size()));
}
BENCHMARK(BM_parse)->Range(1, 1 << 20);

static void BM_manual(benchmark::State& state)
{
    dataheap2::DataChunk data_chunk;
    generate(data_chunk, state);
    for (auto _ : state)
    {
        consume_manual(data_chunk);
        benchmark::DoNotOptimize(consume_sum);
    }
    // actually number of elements, not bytes:
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data_chunk.data_size()));
}
BENCHMARK(BM_manual)->Range(1, 1 << 20);

static void BM_foreach(benchmark::State& state)
{
    dataheap2::DataChunk data_chunk;
    generate(data_chunk, state);
    for (auto _ : state)
    {
        consume_foreach(data_chunk);
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data_chunk.data_size()));
    benchmark::DoNotOptimize(consume_sum);
}
BENCHMARK(BM_foreach)->Range(1, 1 << 20);

static void BM_for(benchmark::State& state)
{
    dataheap2::DataChunk data_chunk;
    generate(data_chunk, state);
    for (auto _ : state)
    {
        consume_for(data_chunk);
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data_chunk.data_size()));
    benchmark::DoNotOptimize(consume_sum);
}
BENCHMARK(BM_for)->Range(1, 1 << 20);

BENCHMARK_MAIN();
