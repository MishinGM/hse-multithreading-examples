#include <array>
#include <atomic>
#include <format>
#include <iostream>

struct AlwaysLockFree {
  std::array<int, 2> data;
};
static_assert(std::atomic<AlwaysLockFree>::is_always_lock_free);

struct NotAlwaysLockFree {
  std::array<int, 3> data;
};
static_assert(std::atomic<NotAlwaysLockFree>::is_always_lock_free == false);

int main() {
  std::atomic<int> x{100};

  // Read operations
  std::cout << "Initial value:" << x << std::endl;
  std::cout << std::format("Initial value: {}", x.load()) << std::endl;

  // Write operations
  x = 1337;
  std::cout << std::format("Value after assignment: {}", x.load()) << std::endl;

  x.store(228);
  std::cout << std::format("Value after store: {}", x.load()) << std::endl;

  // RMW operations
  int previousValue = x.exchange(322);
  std::cout << std::format("Before exchange: {}, after exchange: {}",
                           previousValue, x.load())
            << std::endl;

  previousValue = x.fetch_add(1);
  std::cout << std::format("Before fetch_add: {}, after fetch_add: {}",
                           previousValue, x.load())
            << std::endl;

  previousValue = x.fetch_sub(10);
  std::cout << std::format("Before fetch_sub: {}, after fetch_sub: {}",
                           previousValue, x.load())
            << std::endl;

  int expected{};
  bool exchanged = x.compare_exchange_strong(expected, 322);
  std::cout << std::format("Actual value: {}, exchanged: {}", expected,
                           exchanged)
            << std::endl;

  exchanged = x.compare_exchange_strong(expected, 322);
  std::cout << std::format("Actual value: {}, exchanged: {}", expected,
                           exchanged)
            << std::endl;

  expected = 0;
  exchanged = x.compare_exchange_weak(expected, 322);
  std::cout << std::format("Actual value: {}, exchanged: {}", expected,
                           exchanged)
            << std::endl;

  exchanged = x.compare_exchange_weak(expected, 322);
  std::cout << std::format("Actual value: {}, exchanged: {}", expected,
                           exchanged)
            << std::endl;

  return 0;
}
