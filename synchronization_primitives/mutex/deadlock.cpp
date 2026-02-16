#include <mutex>
#include <thread>

using namespace std::chrono_literals;

void Deadlock() {
  std::mutex m1;
  std::mutex m2;

  std::jthread thread1([&m1, &m2]() {
    std::lock_guard guard1{m1};
    std::this_thread::yield();
    std::lock_guard guard2{m2};
  });

  std::jthread thread2([&m1, &m2]() {
    std::lock_guard guard1{m2};
    std::this_thread::yield();
    std::lock_guard guard2{m1};
  });
}

void NoDeadlock() {
  std::mutex m1;
  std::mutex m2;

  std::jthread thread1([&m1, &m2]() {
    std::lock_guard guard1{m1};
    std::this_thread::yield();
    std::lock_guard guard2{m2};
  });

  std::jthread thread2([&m1, &m2]() {
    std::lock_guard guard1{m1};
    std::this_thread::yield();
    std::lock_guard guard2{m2};
  });
}

int main() {
  for (int run{}; run < 1'000; ++run) {
    Deadlock();
  }

  for (int run{}; run < 1'000; ++run) {
    NoDeadlock();
  }
}