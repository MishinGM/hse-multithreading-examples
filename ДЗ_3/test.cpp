#include "futex_mutex.h"

#include <thread>
#include <vector>
#include <iostream>

int main() {
    FutexMutex mutex;

    int counter = 0;

    std::vector<std::thread> threads;

    for (int t = 0; t < 8; t++) {
        threads.emplace_back([&]() {
            for (int i = 0; i < 100000; i++) {
                mutex.lock();
                counter++;
                mutex.unlock();
            }
        });
    }

    for (auto &th : threads) {
        th.join();
    }

    std::cout << "counter = " << counter << std::endl;
}