#include "cv.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

void test_single_wakeup() {
    TinyCv cv;
    std::mutex mx;
    bool ready = false;
    int awakened = 0;

    std::thread worker([&] {
        std::unique_lock<std::mutex> lock(mx);
        cv.wait(lock, [&] { return ready; });
        ++awakened;
    });

    std::this_thread::sleep_for(30ms);

    {
        std::lock_guard<std::mutex> guard(mx);
        ready = true;
    }
    cv.notify_one();

    worker.join();
    assert(awakened == 1);
}

void test_broadcast_wakeup() {
    TinyCv cv;
    std::mutex mx;
    bool gate_open = false;
    int awakened = 0;
    constexpr int workers_count = 4;
    std::vector<std::thread> workers;

    for (int i = 0; i < workers_count; ++i) {
        workers.emplace_back([&] {
            std::unique_lock<std::mutex> lock(mx);
            cv.wait(lock, [&] { return gate_open; });
            ++awakened;
        });
    }

    std::this_thread::sleep_for(30ms);

    {
        std::lock_guard<std::mutex> guard(mx);
        gate_open = true;
    }
    cv.notify_all();

    for (auto& worker : workers) {
        worker.join();
    }

    assert(awakened == workers_count);
}

void test_timeout_result() {
    TinyCv cv;
    std::mutex mx;
    std::unique_lock<std::mutex> lock(mx);

    const auto started_at = std::chrono::steady_clock::now();
    const auto status = cv.wait_for(lock, 30ms);
    const auto elapsed = std::chrono::steady_clock::now() - started_at;

    assert(status == std::cv_status::timeout);
    assert(elapsed >= 20ms);
}

void test_repeated_signal_rounds() {
    TinyCv cv;
    std::mutex mx;
    bool ready = false;
    constexpr int rounds = 100;

    for (int round = 0; round < rounds; ++round) {
        ready = false;

        std::thread worker([&] {
            std::unique_lock<std::mutex> lock(mx);
            cv.wait(lock, [&] { return ready; });
        });

        std::this_thread::sleep_for(1ms);

        {
            std::lock_guard<std::mutex> guard(mx);
            ready = true;
        }
        cv.notify_one();

        worker.join();
    }
}

int main() {
    test_single_wakeup();
    test_broadcast_wakeup();
    test_timeout_result();
    test_repeated_signal_rounds();
    std::cout << "ok\n";
}