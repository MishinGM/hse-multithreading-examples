#include <iostream>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;

void Livelock() {
  std::timed_mutex m1;
  std::timed_mutex m2;

  std::jthread thread1([&m1, &m2]() {
    std::unique_lock guard1{m1, 5s};
    std::this_thread::yield();
    if (!guard1.owns_lock()) {
      std::cout << "First thread failed to get m1 lock" << std::endl;
      return;
    }

    std::unique_lock guard2{m2, 5s};
    if (!guard2.owns_lock()) {
      std::cout << "First thread failed to get m2 lock" << std::endl;
      return;
    }
  });

  std::jthread thread2([&m1, &m2]() {
    std::unique_lock guard1{m2, 5s};
    std::this_thread::yield();
    if (!guard1.owns_lock()) {
      std::cout << "Second thread failed to get m2 lock" << std::endl;
      return;
    }

    std::unique_lock guard2{m1, 5s};
    if (!guard2.owns_lock()) {
      std::cout << "Second thread failed to get m1 lock" << std::endl;
      return;
    }
  });
}

int main() {
  for (int run{}; run < 1'000; ++run) {
    Livelock();
  }
}