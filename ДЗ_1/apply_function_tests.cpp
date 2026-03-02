#include "apply_function.h"

#include <gtest/gtest.h>
#include <atomic>
#include <numeric>

TEST(ApplyFunction, EmptyVectorDoesNothing)
{
    std::vector<int> v;
    ApplyFunction<int>(v, [](int& x) { x += 1; }, 4);
    EXPECT_TRUE(v.empty());
}

TEST(ApplyFunction, ThreadCountMoreThanElementsClamped)
{
    std::vector<int> v{1, 2, 3};
    ApplyFunction<int>(v, [](int& x) { x *= 2; }, 100);
    EXPECT_EQ(v, (std::vector<int>{2, 4, 6}));
}

TEST(ApplyFunction, NonPositiveThreadCountTreatedAsOne)
{
    std::vector<int> v{1, 2, 3, 4};
    ApplyFunction<int>(v, [](int& x) { x += 5; }, 0);
    EXPECT_EQ(v, (std::vector<int>{6, 7, 8, 9}));

    std::vector<int> u{1, 2, 3, 4};
    ApplyFunction<int>(u, [](int& x) { x += 5; }, -7);
    EXPECT_EQ(u, (std::vector<int>{6, 7, 8, 9}));
}

TEST(ApplyFunction, SingleThreadCorrectness)
{
    std::vector<int> v(1000);
    std::iota(v.begin(), v.end(), 0);

    ApplyFunction<int>(v, [](int& x) { x = x * x; }, 1);

    EXPECT_EQ(v[0], 0);
    EXPECT_EQ(v[1], 1);
    EXPECT_EQ(v[10], 100);
    EXPECT_EQ(v[999], 999 * 999);
}

TEST(ApplyFunction, MultiThreadSameAsSingleThread)
{
    std::vector<int> a(10000);
    std::iota(a.begin(), a.end(), 1);
    std::vector<int> b = a;

    const std::function<void(int&)> f = [](int& x) { x = x * 3 + 1; };

    ApplyFunction<int>(a, f, 1);
    ApplyFunction<int>(b, f, 8);

    EXPECT_EQ(a, b);
}

TEST(ApplyFunction, EachElementProcessedExactlyOnce)
{
    const int n = 2000;
    std::vector<int> v(n, 0);

    std::vector<std::atomic<int>> hits(n);
    for (auto& h : hits) h.store(0);

    ApplyFunction<int>(v, [&](int& x) {
        std::size_t idx = static_cast<std::size_t>(&x - &v[0]);
        hits[idx].fetch_add(1, std::memory_order_relaxed);
        x += 1;
    }, 7);

    for (int i = 0; i < n; ++i) {
        EXPECT_EQ(v[i], 1);
        EXPECT_EQ(hits[static_cast<std::size_t>(i)].load(), 1);
    }
}