#include <format>
#include <iostream>
#include <thread>

int main() {
  int correctSum{};
  int incorrectSum{};

  for (int run{}; run < 1'000; ++run) {
    int x{};

    auto increment = [&x]() {
      for (int i{}; i < 1'000; ++i) {
        ++x;
      }
    };

    std::jthread thread1(increment);
    std::jthread thread2(increment);

    thread1.join();
    thread2.join();

    x == 2'000 ? ++correctSum : ++incorrectSum;
  }

  std::cout << std::format("Correct sum: {}, incorrect sum: {}", correctSum,
                           incorrectSum)
            << std::endl;

  return 0;
}