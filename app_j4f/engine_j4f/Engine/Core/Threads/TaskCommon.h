#pragma once

#include <atomic>
#include <cstdint>
#include <forward_list>
#include <functional>

namespace engine {

    using WorkerTasks = std::forward_list<std::function<void()>>;

    enum class TaskState : uint8_t {
        IDLE = 0u,
        RUN = 1u,
        COMPLETE = 2u,
        CANCELED = 3u
    };

    enum class TaskType : uint8_t { // max value is 7 (using with 1 << TaskType)
        COMMON = 0u,
        USER_CONTROL = 1u, // cancel ����� ������ ������ �� ������� ������������ �������, ��� ThreadPool ������ �������� ����� ������ ������ ��� ��������� � stop()
        MAX_VALUE = 8u
    };

    class CancellationToken final {
        friend class ThreadPool;
        friend class ThreadPool2;
        friend class ITaskHandler;
        friend class TaskBase;
    public:
        inline explicit operator bool() const noexcept {
            return _cancellation_token.test(std::memory_order_relaxed);
        }
    private:
        inline void cancel() noexcept {
            _cancellation_token.test_and_set(std::memory_order_relaxed);
        }

        inline void reset() noexcept {
            _cancellation_token.clear();
        }

        std::atomic_flag _cancellation_token{};
    };

}