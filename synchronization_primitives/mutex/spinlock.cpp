#include <atomic>
#include <format>
#include <iostream>
#include <thread>

class Spinlock {
public:
  void lock() {
    while (m_locked.exchange(true)) {
    }
  }

  void unlock() { m_locked.store(false); }

private:
  std::atomic<bool> m_locked{false};
};

int main() {
  int correctSum{};
  int incorrectSum{};

  for (int run{}; run < 1'000; ++run) {
    int x{};
    Spinlock lock;

    auto increment = [&x, &lock]() {
      for (int i{}; i < 1'000; ++i) {
        std::lock_guard guard{lock};
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