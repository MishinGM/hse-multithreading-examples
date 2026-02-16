#include <mutex>

int main() {
  std::mutex mtx;

  std::lock_guard guard1{mtx};
  std::lock_guard guard2{mtx};

  return 0;
}