#pragma once

#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <ctime>
#include <limits>
#include <linux/futex.h>
#include <mutex>
#include <sys/syscall.h>
#include <unistd.h>

class TinyCv {
public:
    TinyCv() = default;
    TinyCv(const TinyCv&) = delete;
    TinyCv& operator=(const TinyCv&) = delete;

    void notify_one() noexcept {
        advance_epoch();
        wake_waiters(1);
    }

    void notify_all() noexcept {
        advance_epoch();
        wake_waiters(std::numeric_limits<int>::max());
    }

    void wait(std::unique_lock<std::mutex>& lock) {
        const std::uint32_t observed_epoch = current_epoch();
        lock.unlock();
        (void)futex_wait(observed_epoch, nullptr);
        lock.lock();
    }

    template <class Predicate>
    void wait(std::unique_lock<std::mutex>& lock, Predicate predicate) {
        while (!predicate()) {
            wait(lock);
        }
    }

    template <class Rep, class Period>
    std::cv_status wait_for(
        std::unique_lock<std::mutex>& lock,
        const std::chrono::duration<Rep, Period>& timeout) {

        if (timeout <= timeout.zero()) {
            return std::cv_status::timeout;
        }

        const auto deadline =
            std::chrono::steady_clock::now() +
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(timeout);

        const std::uint32_t observed_epoch = current_epoch();
        lock.unlock();

        std::cv_status result = std::cv_status::no_timeout;

        for (;;) {
            const auto now = std::chrono::steady_clock::now();
            if (now >= deadline) {
                result = std::cv_status::timeout;
                break;
            }

            const timespec slice = make_timespec(deadline - now);
            const int rc = futex_wait(observed_epoch, &slice);

            if (rc == 0 || errno == EAGAIN) {
                result = std::cv_status::no_timeout;
                break;
            }

            if (errno == ETIMEDOUT) {
                result = std::cv_status::timeout;
                break;
            }

            if (errno == EINTR) {
                continue;
            }

            result = std::cv_status::no_timeout;
            break;
        }

        lock.lock();
        return result;
    }

    template <class Rep, class Period, class Predicate>
    bool wait_for(std::unique_lock<std::mutex>& lock,
                  const std::chrono::duration<Rep, Period>& timeout,
                  Predicate predicate) {
        const auto deadline =
            std::chrono::steady_clock::now() +
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(timeout);

        while (!predicate()) {
            if (wait_until(lock, deadline) == std::cv_status::timeout) {
                return predicate();
            }
        }
        return true;
    }

    template <class Clock, class Duration>
    std::cv_status wait_until(
        std::unique_lock<std::mutex>& lock,
        const std::chrono::time_point<Clock, Duration>& deadline) {
        const auto now = Clock::now();
        if (deadline <= now) {
            return std::cv_status::timeout;
        }
        return wait_for(lock, deadline - now);
    }

    template <class Clock, class Duration, class Predicate>
    bool wait_until(std::unique_lock<std::mutex>& lock,
                    const std::chrono::time_point<Clock, Duration>& deadline,
                    Predicate predicate) {
        while (!predicate()) {
            if (wait_until(lock, deadline) == std::cv_status::timeout) {
                return predicate();
            }
        }
        return true;
    }

private:
    alignas(std::uint32_t) std::uint32_t epoch_word_{0};

    std::atomic_ref<std::uint32_t> epoch_ref() noexcept {
        return std::atomic_ref<std::uint32_t>(epoch_word_);
    }

    std::uint32_t current_epoch() noexcept {
        return epoch_ref().load(std::memory_order_relaxed);
    }

    void advance_epoch() noexcept {
        epoch_ref().fetch_add(1, std::memory_order_relaxed);
    }

    static timespec make_timespec(std::chrono::steady_clock::duration duration) noexcept {
        if (duration < std::chrono::steady_clock::duration::zero()) {
            duration = std::chrono::steady_clock::duration::zero();
        }

        const auto nanoseconds =
            std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();

        timespec ts{};
        ts.tv_sec = static_cast<time_t>(nanoseconds / 1000000000LL);
        ts.tv_nsec = static_cast<long>(nanoseconds % 1000000000LL);
        return ts;
    }

    int futex_wait(std::uint32_t expected_epoch, const timespec* timeout) noexcept {
        return static_cast<int>(syscall(
            SYS_futex,
            &epoch_word_,
            FUTEX_WAIT_PRIVATE,
            expected_epoch,
            timeout,
            nullptr,
            0));
    }

    void wake_waiters(int count) noexcept {
        (void)syscall(
            SYS_futex,
            &epoch_word_,
            FUTEX_WAKE_PRIVATE,
            count,
            nullptr,
            nullptr,
            0);
    }
};