#include <mutex>
#include <thread>

struct ProtectedData {
public:
  void UpdateData(int newValue) {
    m_mtx.lock();

    if (m_data == 42) {
      return;
    }

    m_data = newValue;
    m_mtx.unlock();
  }

private:
  std::mutex m_mtx;
  int m_data{};
};

int main() {
  ProtectedData data;

  auto updater = [&data](int newValue) {
    std::thread thread([&data, newValue]() { data.UpdateData(newValue); });

    thread.join();
  };

  updater(12);
  updater(42);
  updater(34);
  updater(25);

  return 0;
}