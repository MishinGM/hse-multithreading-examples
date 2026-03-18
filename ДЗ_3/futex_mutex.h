#pragma once

#include <atomic>
#include <cstdint>
#include <cerrno>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

class FutexMutex {
public:
    FutexMutex() = default;
    FutexMutex(const FutexMutex&) = delete;
    FutexMutex& operator=(const FutexMutex&) = delete;

    void lock() {
        std::uint32_t expected = 0;
        if (state_.compare_exchange_strong(
                expected, 1,
                std::memory_order_acquire,
                std::memory_order_relaxed)) {
            return;
        }
        lock_slow();
    }

    bool try_lock() {
        std::uint32_t expected = 0;
        return state_.compare_exchange_strong(
            expected, 1,
            std::memory_order_acquire,
            std::memory_order_relaxed
        );
    }

    void unlock() {
        
        const std::uint32_t prev = state_.fetch_sub(1, std::memory_order_release);

        if (prev != 1) {
            state_.store(0, std::memory_order_release);
            futex_wake_one();
        }
    }

private:

    std::atomic<std::uint32_t> state_{0};

    static int futex_wait(std::atomic<std::uint32_t>* addr, std::uint32_t expected) {
        return static_cast<int>(syscall(
            SYS_futex,
            reinterpret_cast<std::uint32_t*>(addr),
            FUTEX_WAIT_PRIVATE,
            expected,
            nullptr,
            nullptr,
            0
        ));
    }

    void futex_wake_one() {
        syscall(
            SYS_futex,
            reinterpret_cast<std::uint32_t*>(&state_),
            FUTEX_WAKE_PRIVATE,
            1,
            nullptr,
            nullptr,
            0
        );
    }

    void lock_slow() {
        while (true) {
          
            const std::uint32_t prev = state_.exchange(2, std::memory_order_acquire);
            if (prev == 0) {
                return;
            }

           
            while (state_.load(std::memory_order_relaxed) == 2) {
                const int rc = futex_wait(&state_, 2);
                if (rc == -1 && errno != EAGAIN && errno != EINTR) {
                  
                }
            }
        }
    }
};