#include "futex_mutex.h"

#include <thread>
#include <vector>
#include <chrono>
#include <iostream>

int main() {

    FutexMutex mutex;

    int counter = 0;

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;

    for (int t = 0; t < 8; t++) {
        threads.emplace_back([&]() {
            for (int i = 0; i < 2000000; i++) {
                mutex.lock();
                counter++;
                mutex.unlock();
            }
        });
    }

    for (auto &th : threads) {
        th.join();
    }

    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "counter = " << counter << std::endl;
    std::cout << "time = "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n";
}