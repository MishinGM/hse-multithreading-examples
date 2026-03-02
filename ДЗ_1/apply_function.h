#pragma once

#include <vector>
#include <functional>
#include <thread>
#include <cstddef>

template <typename T>
void ApplyFunction(std::vector<T>& data,
                   const std::function<void(T&)>& transform,
                   const int threadCount = 1)
{
    const std::size_t n = data.size();
    if (n == 0) return;

    int tc = threadCount;
    if (tc <= 0) tc = 1;
    if (static_cast<std::size_t>(tc) > n) tc = static_cast<int>(n);

    if (tc == 1) {
        for (auto& x : data) transform(x);
        return;
    }

    std::vector<std::thread> threads;
    threads.reserve(static_cast<std::size_t>(tc));

    const std::size_t base = n / static_cast<std::size_t>(tc);
    const std::size_t rem  = n % static_cast<std::size_t>(tc);

    std::size_t begin = 0;
    for (int i = 0; i < tc; ++i) {
        const std::size_t chunk = base + (static_cast<std::size_t>(i) < rem ? 1 : 0);
        const std::size_t end = begin + chunk;

        threads.emplace_back([&data, &transform, begin, end]() {
            for (std::size_t j = begin; j < end; ++j) {
                transform(data[j]);
            }
        });

        begin = end;
    }

    for (auto& th : threads) th.join();
}