#pragma once

#include <atomic>
#include <cstdint>

namespace engine {

    enum class TaskState : uint8_t {
        IDLE = 0,
        RUN = 1,
        COMPLETE = 2,
        CANCELED = 3
    };

    enum class TaskType : uint8_t { // max value is 7 (using with 1 << TaskType)
        COMMON = 0,
        USER_CONTROL = 1, // cancel будет вызван только на стороне использующей стороны, сам ThreadPool сможет отменить такую задачу только при остановке в stop()
        _MAX_VALUE = 8
    };

    class CancellationToken final {
        friend class ThreadPool;
        friend class ThreadPool2;
        friend class ITaskHandler;
        friend class TaskBase;
    public:
        inline operator bool() const noexcept {
            return _cancellation_token.test(std::memory_order_consume);
        }
    private:
        inline void cancel() noexcept {
            _cancellation_token.test_and_set(std::memory_order_release);
        }

        inline void reset() noexcept {
            _cancellation_token.clear();
        }

        std::atomic_flag _cancellation_token{};
    };

}