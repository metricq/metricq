// This is not part of the project.
// This is a completely independent test
// Also I can't be bothered to copy over a FindLZ4.cmake

#include <lz4.h>

#include <benchmark/benchmark.h>

#include <iostream>
#include <random>
#include <vector>

#include <cstdint>

struct TimeValue
{
    int64_t timestamp;
    double value;
};

std::uniform_real_distribution<double> distribution(5.0, 200.0);
int64_t timestamp_base = 1519832293179227888ll;
int64_t timestamp_step0 = 1000000000ll / 1210000;

struct generate_cont
{
    std::vector<TimeValue> operator()(benchmark::State& state)
    {
        std::vector<TimeValue> g;
        g.reserve(state.range(0));
        std::mt19937 generator;
        auto timestamp_step = state.range(1);

        int64_t timestamp = timestamp_base;
        for (size_t i = 0; i < state.range(0); i++)
        {
            g.push_back({ timestamp, distribution(generator) });
            timestamp += timestamp_step;
        }
        return g;
    }
};

struct generate_base
{
    std::vector<TimeValue> operator()(benchmark::State& state)
    {
        std::vector<TimeValue> g;
        g.reserve(state.range(0));
        std::mt19937 generator;
        auto timestamp_step = state.range(1);

        g.push_back({ timestamp_base, distribution(generator) });
        int64_t timestamp = timestamp_step;
        for (size_t i = 1; i < state.range(0); i++)
        {
            g.push_back({ timestamp, distribution(generator) });
            timestamp += timestamp_step;
        }
        return g;
    }
};

struct generate_delta
{
    std::vector<TimeValue> operator()(benchmark::State& state)
    {
        std::vector<TimeValue> g;
        g.reserve(state.range(0));
        std::mt19937 generator;
        auto timestamp_step = state.range(1);

        g.push_back({ timestamp_base, distribution(generator) });
        for (size_t i = 1; i < state.range(0); i++)
        {
            g.push_back({ timestamp_step, distribution(generator) });
        }
        return g;
    }
};

template <typename Generator>
static void BM_compress(benchmark::State& state)
{
    std::vector<TimeValue> in = Generator()(state);
    std::vector<char> out;
    out.resize(32 + in.size() * sizeof(TimeValue));
    int out_size;
    size_t in_size = in.size() * sizeof(TimeValue);
    for (auto _ : state)
    {
        out_size = LZ4_compress_fast(reinterpret_cast<const char*>(in.data()), out.data(), in_size,
                                     out.size(), state.range(2));
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range(0)));
    state.counters["compressed_size"] = out_size;
    state.counters["compression_rate"] = static_cast<double>(out_size) / in_size;
    // std::cout << in_size << " -> " << out_size << " [" << static_cast<double>(out_size) / in_size
    //          << "]\n";
}

template <typename Generator>
static void BM_decompress(benchmark::State& state)
{
    std::vector<TimeValue> in = Generator()(state);
    std::vector<char> compressed;
    compressed.resize(512 + in.size() * sizeof(TimeValue));
    size_t in_size = in.size() * sizeof(TimeValue);
    int compressed_size =
        LZ4_compress_fast(reinterpret_cast<const char*>(in.data()), compressed.data(), in_size,
                          compressed.size(), state.range(2));
    std::vector<TimeValue> decompressed;
    decompressed.resize(in.size());

    for (auto _ : state)
    {
        LZ4_decompress_safe(compressed.data(), reinterpret_cast<char*>(decompressed.data()),
                            compressed_size, in_size);
    }

    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range(0)));
    state.counters["compression_rate"] = static_cast<double>(compressed_size) / in_size;
    // std::cout << in_size << " -> " << out_size << " [" << static_cast<double>(out_size) / in_size
    //          << "]\n";
}

static void CustomArguments(benchmark::internal::Benchmark* b)
{
    for (int size = 1; size <= 4096; size *= 8)
    {
        for (int timestamp_step : { 1000000000ll / 1210000, 1000000000ll / 20, 1000000000ll })
        {
            for (int compression_level : { 1, 5, 9 })
            {
                b->Args({ size, timestamp_step, compression_level });
            }
        }
    }
}

BENCHMARK_TEMPLATE(BM_compress, generate_cont)->Apply(CustomArguments);
BENCHMARK_TEMPLATE(BM_compress, generate_base)->Apply(CustomArguments);
BENCHMARK_TEMPLATE(BM_compress, generate_delta)->Apply(CustomArguments);

BENCHMARK_TEMPLATE(BM_decompress, generate_cont)->Apply(CustomArguments);
BENCHMARK_TEMPLATE(BM_decompress, generate_base)->Apply(CustomArguments);
BENCHMARK_TEMPLATE(BM_decompress, generate_delta)->Apply(CustomArguments);

BENCHMARK_MAIN();