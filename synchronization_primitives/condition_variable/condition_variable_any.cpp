#include <condition_variable>
#include <format>
#include <iostream>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

class Worker {
public:
  Worker()
      : m_workerThread([this](std::stop_token stopToken) noexcept {
          WorkerThread(stopToken);
        }) {}

  void AddTask(int task) {
    {
      std::lock_guard guard{m_mtx};
      m_tasks.push_back(task);
    }

    m_newTaskAdded.notify_one();
  }

private:
  void WorkerThread(std::stop_token stopToken) noexcept {
    while (!stopToken.stop_requested()) {
      std::vector<int> tasks;

      try {
        {
          std::unique_lock lock{m_mtx};
          m_newTaskAdded.wait(lock, [this]() { return !m_tasks.empty(); });
          std::swap(m_tasks, tasks);
        }

        for (int task : tasks) {
          std::cout << std::format("Executing task #{}", task) << std::endl;
        }
      } catch (...) {
        std::cerr << "Exception in worker thread" << std::endl;
      }
    }
  }

private:
  std::mutex m_mtx;
  std::condition_variable_any m_newTaskAdded;
  std::vector<int> m_tasks;
  std::jthread m_workerThread;
};

int main() {
  Worker worker;

  worker.AddTask(10);

  std::this_thread::sleep_for(3s);

  worker.AddTask(4);
  worker.AddTask(5);

  std::this_thread::sleep_for(2s);

  return 0;
}
