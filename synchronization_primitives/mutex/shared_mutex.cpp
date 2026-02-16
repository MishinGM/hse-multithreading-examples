#include <mutex>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

class DataManager {
public:
    void ChangeData(std::vector<int> newData) {
        std::lock_guard guard{m_mtx};
        m_data = std::move(newData);
    }

    std::vector<int> GetCopy() {
        std::lock_guard guard{m_mtx};

        // Simulate large data size
        std::this_thread::sleep_for(200ms);

        return m_data;
    }

private:
    std::mutex m_mtx;
    std::vector<int> m_data;
};

int main() {
    DataManager data;

    auto dataAccessor = [&data]() {
        for (int i{}; i < 5; ++i) {
            std::ignore = data.GetCopy();
        }
    };

    std::jthread thread1(dataAccessor);
    std::jthread thread2(dataAccessor);

    return 0;
}
