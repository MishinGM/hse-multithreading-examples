#include <atomic>
#include <format>
#include <iostream>
#include <thread>

int main() {
  int correctValue{};
  int incorrectValue{};

  for (int run{}; run < 10'000; ++run) {
    std::atomic<int> x{100};
    static_assert(std::atomic<int>::is_always_lock_free);

    auto decrement = [&x]() {
      int expected = x.load();
      std::this_thread::yield();
      while (expected > 0) {
        std::this_thread::yield();
        x.compare_exchange_weak(expected, expected - 100);
      }
    };

    std::jthread thread1(decrement);
    std::jthread thread2(decrement);

    thread1.join();
    thread2.join();

    x == 0 ? ++correctValue : ++incorrectValue;
  }

  std::cout << std::format("Correct value: {}, incorrect value: {}",
                           correctValue, incorrectValue)
            << std::endl;

  return 0;
}