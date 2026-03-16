#include "apply_function.h"

#include <benchmark/benchmark.h>
#include <vector>
#include <cstdint>

static void LightTransform(std::int64_t& x) { x += 1; }

static void HeavyTransform(std::int64_t& x)
{
    std::uint64_t v = static_cast<std::uint64_t>(x) + 0x9e3779b97f4a7c15ULL;
    for (int i = 0; i < 200; ++i) {
        v ^= v >> 12;
        v ^= v << 25;
        v ^= v >> 27;
        v *= 2685821657736338717ULL;
    }
    x = static_cast<std::int64_t>(v);
}

static std::vector<std::int64_t> MakeData(std::size_t n)
{
    std::vector<std::int64_t> v(n);
    for (std::size_t i = 0; i < n; ++i) v[i] = static_cast<std::int64_t>(i);
    return v;
}


static void BM_Light_SingleThread(benchmark::State& state)
{
    const std::size_t n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        auto data = MakeData(n);
        ApplyFunction<std::int64_t>(data, [](std::int64_t& x) { LightTransform(x); }, 1);
        benchmark::DoNotOptimize(data.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Light_SingleThread)->Arg(1'000)->Arg(5'000)->Arg(10'000);

static void BM_Light_MultiThread(benchmark::State& state)
{
    const std::size_t n = static_cast<std::size_t>(state.range(0));
    const int threads = static_cast<int>(state.range(1));
    for (auto _ : state) {
        auto data = MakeData(n);
        ApplyFunction<std::int64_t>(data, [](std::int64_t& x) { LightTransform(x); }, threads);
        benchmark::DoNotOptimize(data.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Light_MultiThread)->Args({1'000, 8})->Args({5'000, 8})->Args({10'000, 8});


static void BM_Heavy_SingleThread(benchmark::State& state)
{
    const std::size_t n = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        auto data = MakeData(n);
        ApplyFunction<std::int64_t>(data, [](std::int64_t& x) { HeavyTransform(x); }, 1);
        benchmark::DoNotOptimize(data.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Heavy_SingleThread)->Arg(200'000)->Arg(500'000)->Arg(1'000'000);

static void BM_Heavy_MultiThread(benchmark::State& state)
{
    const std::size_t n = static_cast<std::size_t>(state.range(0));
    const int threads = static_cast<int>(state.range(1));
    for (auto _ : state) {
        auto data = MakeData(n);
        ApplyFunction<std::int64_t>(data, [](std::int64_t& x) { HeavyTransform(x); }, threads);
        benchmark::DoNotOptimize(data.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Heavy_MultiThread)->Args({200'000, 8})
                               ->Args({500'000, 8})
                               ->Args({1'000'000, 8});

BENCHMARK_MAIN();