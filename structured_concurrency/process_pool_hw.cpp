#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <concepts>
#include <condition_variable>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace sc {

constexpr std::size_t kMaxPayloadSize = 256;
constexpr std::size_t kMaxErrorSize = 256;

struct TaskMessage;
struct ResponseMessage;
using RunnerFn = void (*)(const TaskMessage&, ResponseMessage&) noexcept;

struct TaskMessage {
    enum class Kind : std::uint32_t {
        Run = 1,
        Stop = 2,
    };

    Kind kind{Kind::Run};
    std::uint64_t task_id{};
    std::uintptr_t function_ptr{};
    std::uintptr_t runner_ptr{};
    std::uint32_t arg_size{};
    std::array<std::byte, kMaxPayloadSize> arg{};
};

struct ResponseMessage {
    std::uint64_t task_id{};
    bool has_error{false};
    std::uint32_t result_size{};
    std::array<std::byte, kMaxPayloadSize> result{};
    char error[kMaxErrorSize]{};
};

bool WriteExact(const int fd, const void* data, std::size_t size) {
    const auto* ptr = static_cast<const std::byte*>(data);
    std::size_t written = 0;

    while (written < size) {
        const ssize_t rc = ::write(fd, ptr + written, size - written);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        written += static_cast<std::size_t>(rc);
    }

    return true;
}

bool ReadExact(const int fd, void* data, std::size_t size) {
    auto* ptr = static_cast<std::byte*>(data);
    std::size_t read_bytes = 0;

    while (read_bytes < size) {
        const ssize_t rc = ::read(fd, ptr + read_bytes, size - read_bytes);
        if (rc == 0) {
            return false;
        }
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        read_bytes += static_cast<std::size_t>(rc);
    }

    return true;
}

class ISharedState {
public:
    virtual ~ISharedState() = default;
    virtual void Fulfill(const ResponseMessage& response) = 0;
    virtual void Fail(std::string error) = 0;
};

template <typename T>
class SharedState final : public ISharedState {
public:
    void Fulfill(const ResponseMessage& response) override {
        std::lock_guard lock(mutex_);
        if (ready_) {
            return;
        }

        if (response.has_error) {
            error_ = response.error;
        } else {
            if (response.result_size != sizeof(T)) {
                error_ = "Invalid result size from worker";
            } else {
                T value{};
                std::memcpy(&value, response.result.data(), sizeof(T));
                value_ = std::move(value);
            }
        }

        ready_ = true;
        cv_.notify_all();
    }

    void Fail(std::string error) override {
        std::lock_guard lock(mutex_);
        if (ready_) {
            return;
        }
        error_ = std::move(error);
        ready_ = true;
        cv_.notify_all();
    }

    void Wait() const {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this] { return ready_; });
    }

    bool IsReady() const {
        std::lock_guard lock(mutex_);
        return ready_;
    }

    T Get() {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this] { return ready_; });

        if (consumed_) {
            throw std::runtime_error("Future result was already retrieved");
        }
        consumed_ = true;

        if (error_.has_value()) {
            throw std::runtime_error(*error_);
        }

        return std::move(*value_);
    }

private:
    mutable std::mutex mutex_;
    mutable std::condition_variable cv_;
    bool ready_{false};
    bool consumed_{false};
    std::optional<T> value_;
    std::optional<std::string> error_;
};

template <>
class SharedState<void> final : public ISharedState {
public:
    void Fulfill(const ResponseMessage& response) override {
        std::lock_guard lock(mutex_);
        if (ready_) {
            return;
        }

        if (response.has_error) {
            error_ = response.error;
        }

        ready_ = true;
        cv_.notify_all();
    }

    void Fail(std::string error) override {
        std::lock_guard lock(mutex_);
        if (ready_) {
            return;
        }
        error_ = std::move(error);
        ready_ = true;
        cv_.notify_all();
    }

    void Wait() const {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this] { return ready_; });
    }

    bool IsReady() const {
        std::lock_guard lock(mutex_);
        return ready_;
    }

    void Get() {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this] { return ready_; });

        if (consumed_) {
            throw std::runtime_error("Future result was already retrieved");
        }
        consumed_ = true;

        if (error_.has_value()) {
            throw std::runtime_error(*error_);
        }
    }

private:
    mutable std::mutex mutex_;
    mutable std::condition_variable cv_;
    bool ready_{false};
    bool consumed_{false};
    std::optional<std::string> error_;
};

template <typename T>
class Future {
public:
    Future() = default;
    explicit Future(std::shared_ptr<SharedState<T>> state) : state_(std::move(state)) {}

    void Wait() const {
        EnsureValid();
        state_->Wait();
    }

    bool IsReady() const {
        EnsureValid();
        return state_->IsReady();
    }

    T Get() {
        EnsureValid();
        return state_->Get();
    }

private:
    void EnsureValid() const {
        if (!state_) {
            throw std::runtime_error("Future has no shared state");
        }
    }

    std::shared_ptr<SharedState<T>> state_;
};

template <typename T>
concept TriviallySerializable =
    std::is_trivially_copyable_v<T> && (sizeof(T) <= kMaxPayloadSize);

template <typename Fn, typename Arg>
concept ProcessCallable = requires(Fn fn, Arg arg) {
    { (+fn)(arg) };
};

template <typename Result, typename Arg>
void RunTaskImpl(const TaskMessage& task, ResponseMessage& response) noexcept {
    using FunctionPtr = Result (*)(Arg);
    const auto fn = reinterpret_cast<FunctionPtr>(task.function_ptr);

    try {
        Arg argument{};
        std::memcpy(&argument, task.arg.data(), sizeof(Arg));

        if constexpr (std::is_void_v<Result>) {
            fn(argument);
            response.result_size = 0;
        } else {
            Result result = fn(argument);
            response.result_size = sizeof(Result);
            std::memcpy(response.result.data(), &result, sizeof(Result));
        }
    } catch (const std::exception& e) {
        response.has_error = true;
        std::snprintf(response.error, sizeof(response.error), "%s", e.what());
    } catch (...) {
        response.has_error = true;
        std::snprintf(response.error, sizeof(response.error), "%s", "Unknown exception");
    }
}

class ProcessPool {
public:
    explicit ProcessPool(std::size_t process_count) {
        if (process_count == 0) {
            throw std::invalid_argument("ProcessPool must contain at least one process");
        }

        std::signal(SIGPIPE, SIG_IGN);
        workers_.reserve(process_count);

        for (std::size_t i = 0; i < process_count; ++i) {
            CreateWorker();
        }
    }

    ProcessPool(const ProcessPool&) = delete;
    ProcessPool& operator=(const ProcessPool&) = delete;

    ~ProcessPool() {
        Shutdown();
    }

    template <typename Fn, typename Arg>
        requires ProcessCallable<Fn, Arg> && TriviallySerializable<Arg>
    auto Submit(Fn fn, Arg arg) -> Future<std::invoke_result_t<decltype(+fn), Arg>> {
        using Result = std::invoke_result_t<decltype(+fn), Arg>;
        static_assert(std::is_void_v<Result> || TriviallySerializable<Result>,
                      "Result type must be void or trivially copyable and fit into the payload");

        using FunctionPtr = decltype(+fn);

        if (stopped_.load()) {
            throw std::runtime_error("ProcessPool is already stopped");
        }

        TaskMessage task{};
        task.kind = TaskMessage::Kind::Run;
        task.task_id = next_task_id_.fetch_add(1);
        task.function_ptr = reinterpret_cast<std::uintptr_t>(static_cast<FunctionPtr>(+fn));
        task.runner_ptr = reinterpret_cast<std::uintptr_t>(&RunTaskImpl<Result, Arg>);
        task.arg_size = sizeof(Arg);
        std::memcpy(task.arg.data(), &arg, sizeof(Arg));

        auto state = std::make_shared<SharedState<Result>>();
        {
            std::lock_guard lock(states_mutex_);
            states_.emplace(task.task_id, state);
        }

        Worker& worker = workers_.at(next_worker_.fetch_add(1) % workers_.size());
        if (!WriteExact(worker.request_write_fd, &task, sizeof(task))) {
            std::lock_guard lock(states_mutex_);
            states_.erase(task.task_id);
            throw std::runtime_error("Failed to submit task to worker process");
        }

        return Future<Result>{std::move(state)};
    }

private:
    struct Worker {
        int request_write_fd{-1};
        int response_read_fd{-1};
        pid_t pid{-1};
        std::jthread reader;
    };

    void CreateWorker() {
        int request_pipe[2]{};
        int response_pipe[2]{};
        if (::pipe(request_pipe) != 0 || ::pipe(response_pipe) != 0) {
            throw std::runtime_error("pipe() failed");
        }

        const pid_t pid = ::fork();
        if (pid < 0) {
            throw std::runtime_error("fork() failed");
        }

        if (pid == 0) {
            ::close(request_pipe[1]);
            ::close(response_pipe[0]);
            WorkerMain(request_pipe[0], response_pipe[1]);
        }

        ::close(request_pipe[0]);
        ::close(response_pipe[1]);

        Worker worker{};
        worker.request_write_fd = request_pipe[1];
        worker.response_read_fd = response_pipe[0];
        worker.pid = pid;
        worker.reader = std::jthread([this, fd = worker.response_read_fd](std::stop_token) {
            ReaderLoop(fd);
        });
        workers_.push_back(std::move(worker));
    }

    [[noreturn]] static void WorkerMain(const int request_read_fd, const int response_write_fd) {
        while (true) {
            TaskMessage task{};
            if (!ReadExact(request_read_fd, &task, sizeof(task))) {
                break;
            }
            if (task.kind == TaskMessage::Kind::Stop) {
                break;
            }

            ResponseMessage response{};
            response.task_id = task.task_id;
            const auto runner = reinterpret_cast<RunnerFn>(task.runner_ptr);
            runner(task, response);

            if (!WriteExact(response_write_fd, &response, sizeof(response))) {
                break;
            }
        }

        ::close(request_read_fd);
        ::close(response_write_fd);
        _exit(0);
    }

    void ReaderLoop(const int response_read_fd) {
        while (true) {
            ResponseMessage response{};
            if (!ReadExact(response_read_fd, &response, sizeof(response))) {
                break;
            }

            std::shared_ptr<ISharedState> state;
            {
                std::lock_guard lock(states_mutex_);
                auto it = states_.find(response.task_id);
                if (it != states_.end()) {
                    state = it->second;
                    states_.erase(it);
                }
            }

            if (state) {
                state->Fulfill(response);
            }
        }
    }

    void Shutdown() {
        if (stopped_.exchange(true)) {
            return;
        }

        TaskMessage stop{};
        stop.kind = TaskMessage::Kind::Stop;

        for (auto& worker : workers_) {
            if (worker.request_write_fd != -1) {
                WriteExact(worker.request_write_fd, &stop, sizeof(stop));
                ::close(worker.request_write_fd);
                worker.request_write_fd = -1;
            }
        }

        for (auto& worker : workers_) {
            if (worker.pid > 0) {
                int status = 0;
                ::waitpid(worker.pid, &status, 0);
            }
            if (worker.response_read_fd != -1) {
                ::close(worker.response_read_fd);
                worker.response_read_fd = -1;
            }
        }

        FailRemainingTasks("ProcessPool was stopped before task completion");
    }

    void FailRemainingTasks(const std::string& error) {
        std::unordered_map<std::uint64_t, std::shared_ptr<ISharedState>> states;
        {
            std::lock_guard lock(states_mutex_);
            states.swap(states_);
        }

        for (auto& [_, state] : states) {
            state->Fail(error);
        }
    }

    std::vector<Worker> workers_;
    std::mutex states_mutex_;
    std::unordered_map<std::uint64_t, std::shared_ptr<ISharedState>> states_;
    std::atomic<std::uint64_t> next_task_id_{1};
    std::atomic<std::size_t> next_worker_{0};
    std::atomic<bool> stopped_{false};
};

}  

struct NumberPair {
    int a;
    int b;
};

int Square(int x) {
    ::sleep(1);
    return x * x;
}

int SumPair(NumberPair pair) {
    return pair.a + pair.b;
}

void ThrowOnNegative(int x) {
    if (x < 0) {
        throw std::runtime_error("Negative value is not allowed");
    }
}

int main() {
    sc::ProcessPool pool{3};

    auto future1 = pool.Submit(Square, 11);
    auto future2 = pool.Submit(SumPair, NumberPair{10, 32});
    auto future3 = pool.Submit(ThrowOnNegative, -1);

    std::cout << "future1 ready right after submit: " << std::boolalpha
              << future1.IsReady() << '\n';

    std::cout << "Square result: " << future1.Get() << '\n';
    std::cout << "Sum result: " << future2.Get() << '\n';

    try {
        future3.Get();
    } catch (const std::exception& e) {
        std::cout << "Exception from worker: " << e.what() << '\n';
    }

    return 0;
}