#include <barrier>
#include <iostream>
#include <format>
#include <thread>
#include <vector>

int main() {
    static constexpr int WorkersCount = 3;
    std::barrier workersFinished{WorkersCount, [](){
        std::cout << "All workers reached the barried" << std::endl;
    }};

    std::vector<std::jthread> workers;
    for (int worker{}; worker < WorkersCount; ++worker) {
        workers.emplace_back([worker, &workersFinished](){
            // Simulate hard work
            std::this_thread::sleep_for(std::chrono::seconds{WorkersCount - worker});
            std::cout << std::format("Worker #{} finished", worker + 1) << std::endl;
            workersFinished.arrive_and_wait();

            // Simulate clean up
            std::this_thread::sleep_for(std::chrono::seconds{worker + 1});
            std::cout << std::format("Worker #{} finished cleaning up", worker + 1) << std::endl;
            workersFinished.arrive_and_wait();
        });
    }

    return 0;
}
