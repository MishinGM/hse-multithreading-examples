#include <coroutine>
#include <deque>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

class Scheduler {
public:
    void Schedule(std::coroutine_handle<> handle) {
        if (handle) {
            ready_.push_back(handle);
        }
    }

    void Run() {
        while (!ready_.empty()) {
            auto handle = ready_.front();
            ready_.pop_front();

            if (!handle.done()) {
                handle.resume();
            }
        }
    }

private:
    std::deque<std::coroutine_handle<>> ready_;
};

class Task {
public:
    struct promise_type;
    using Handle = std::coroutine_handle<promise_type>;

    Task() = default;

    explicit Task(Handle handle) : handle_(handle) {}

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task(Task&& other) noexcept : handle_(std::exchange(other.handle_, {})) {}

    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (handle_) {
                handle_.destroy();
            }
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }

    ~Task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    void Start(Scheduler& scheduler) {
        if (!handle_) {
            throw std::runtime_error("Task has no coroutine handle");
        }

        handle_.promise().scheduler = &scheduler;
        scheduler.Schedule(handle_);
    }

    void RethrowIfFailed() const {
        if (handle_ && handle_.promise().exception) {
            std::rethrow_exception(handle_.promise().exception);
        }
    }

    struct Awaiter {
        Handle handle;

        bool await_ready() const noexcept {
            return !handle || handle.done();
        }

        void await_suspend(std::coroutine_handle<promise_type> awaiting) noexcept {
            auto& child_promise = handle.promise();
            child_promise.scheduler = awaiting.promise().scheduler;
            child_promise.continuation = awaiting;
            child_promise.scheduler->Schedule(handle);
        }

        void await_resume() {
            if (!handle) {
                return;
            }

            if (handle.promise().exception) {
                auto exception = handle.promise().exception;
                handle.destroy();
                handle = {};
                std::rethrow_exception(exception);
            }

            handle.destroy();
            handle = {};
        }
    };

    Awaiter operator co_await() && noexcept {
        return Awaiter{std::exchange(handle_, {})};
    }

    struct promise_type {
        Scheduler* scheduler = nullptr;
        std::coroutine_handle<> continuation{};
        std::exception_ptr exception{};

        Task get_return_object() {
            return Task{Handle::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept {
            return {};
        }

        struct FinalAwaiter {
            bool await_ready() const noexcept {
                return false;
            }

            template <typename Promise>
            void await_suspend(std::coroutine_handle<Promise> handle) const noexcept {
                auto continuation = handle.promise().continuation;
                auto* scheduler = handle.promise().scheduler;

                if (continuation && scheduler) {
                    scheduler->Schedule(continuation);
                }
            }

            void await_resume() const noexcept {}
        };

        FinalAwaiter final_suspend() noexcept {
            return {};
        }

        void return_void() noexcept {}

        void unhandled_exception() noexcept {
            exception = std::current_exception();
        }
    };

private:
    Handle handle_{};
};

struct YieldToScheduler {
    Scheduler* scheduler;

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(std::coroutine_handle<> handle) const noexcept {
        scheduler->Schedule(handle);
    }

    void await_resume() const noexcept {}
};

struct Graph {
    explicit Graph(std::size_t vertex_count) : adjacency(vertex_count) {}

    void AddEdge(int from, int to) {
        adjacency.at(from).push_back(to);
    }

    std::size_t Size() const {
        return adjacency.size();
    }

    std::vector<std::vector<int>> adjacency;
};

struct DfsContext {
    const Graph& graph;
    Scheduler& scheduler;
    std::vector<bool> visited;
    std::vector<int> order;

    DfsContext(const Graph& graph, Scheduler& scheduler)
        : graph(graph), scheduler(scheduler), visited(graph.Size(), false) {}
};

Task DfsVertex(int vertex, DfsContext& context) {
    if (context.visited.at(vertex)) {
        co_return;
    }

    context.visited[vertex] = true;
    context.order.push_back(vertex);
    std::cout << "visit " << vertex << '\n';

   
    co_await YieldToScheduler{&context.scheduler};

    for (int to : context.graph.adjacency.at(vertex)) {
        if (!context.visited.at(to)) {
           
            co_await DfsVertex(to, context);
        }

        
        co_await YieldToScheduler{&context.scheduler};
    }
}

Task DfsAllComponents(DfsContext& context) {
    for (int vertex = 0; vertex < static_cast<int>(context.graph.Size()); ++vertex) {
        if (!context.visited[vertex]) {
            co_await DfsVertex(vertex, context);
            co_await YieldToScheduler{&context.scheduler};
        }
    }
}

int main() {
    Graph graph(7);
    graph.AddEdge(0, 1);
    graph.AddEdge(0, 2);
    graph.AddEdge(1, 3);
    graph.AddEdge(1, 4);
    graph.AddEdge(2, 5);
    graph.AddEdge(5, 6);

    Scheduler scheduler;
    DfsContext context(graph, scheduler);

    Task root = DfsAllComponents(context);
    root.Start(scheduler);
    scheduler.Run();
    root.RethrowIfFailed();

    std::cout << "DFS order:";
    for (int vertex : context.order) {
        std::cout << ' ' << vertex;
    }
    std::cout << '\n';

    return 0;
}