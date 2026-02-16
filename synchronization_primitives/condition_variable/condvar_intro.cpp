#include <condition_variable>
#include <format>
#include <iostream>
#include <thread>

class CVLock {
public:
  void lock() {
    std::unique_lock lock{m_mtx};

    if (m_locked == true) {
        m_mutexUnlocked.wait(lock);
    }

    m_locked = true;
  }

  void unlock() {
    std::unique_lock lock{m_mtx};
    m_locked = false;
    m_mutexUnlocked.notify_one();
  }

private:
  std::mutex m_mtx;
  std::condition_variable m_mutexUnlocked;
  bool m_locked{};
};

int main() {
  int correctSum{};
  int incorrectSum{};

  for (int run{}; run < 1'000; ++run) {
    int x{};
    CVLock lock;

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