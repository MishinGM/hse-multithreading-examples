#include <iostream>
#include <format>
#include <latch>
#include <thread>
#include <vector>

int main() {
    static constexpr int WorkersCount = 3;
    std::latch workersFinished{WorkersCount};

    std::vector<std::jthread> workers;
    for (int worker{}; worker < WorkersCount; ++worker) {
        workers.emplace_back([worker, &workersFinished](){
            // Simulate hard work
            std::this_thread::sleep_for(std::chrono::seconds{WorkersCount - worker});
            std::cout << std::format("Worker #{} finished", worker + 1) << std::endl;
            workersFinished.count_down();
        });
    }

    workersFinished.wait();

    std::cout << "All workers finished" << std::endl;

    return 0;
}
