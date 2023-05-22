#pragma once

#include "Synchronisations.h"
#include "TaskCommon.h"
#include <condition_variable>
#include <functional>
#include <optional>
#include <atomic>
#include <cstdint>

namespace engine {

    class task_control_block {
        using type = task_control_block;
        using counter_type = std::atomic_uint32_t;

    public:
        inline uint32_t _decrease_counter() noexcept { return m_counter.fetch_sub(1, std::memory_order_release) - 1; }
        inline uint32_t _increase_counter() noexcept { return m_counter.fetch_add(1, std::memory_order_release) + 1; }
        [[nodiscard]] inline uint32_t _use_count() const noexcept { return m_counter.load(std::memory_order_relaxed); }

    private:
        std::atomic_uint32_t m_counter = 0;
    };

    template <typename T>
    class Task2;

    class TaskBase : public task_control_block {
        using Locker = SpinLock;
        using CondVar = std::condition_variable_any;
        friend class ThreadPool2;
    public:
        TaskBase() = default;
        virtual ~TaskBase() {
            cancel();
        }

        [[nodiscard]] inline TaskState state() const noexcept {
            return _state.load(std::memory_order_acquire);
        }

        inline void wait() noexcept {
            std::unique_lock<Locker> lock(_locker);
            _condition.wait(lock, [this] {
                const auto s = state();
                return s == TaskState::COMPLETE || s == TaskState::CANCELED;
            });
        }

        inline void cancel() noexcept {
            TaskState cancelled = TaskState::CANCELED;
            if (!_state.compare_exchange_strong(cancelled, TaskState::CANCELED, std::memory_order_release, std::memory_order_relaxed)) {
                _token.cancel();
                notify();
            }
        }

        inline void notify() noexcept {
            _condition.notify_one();
        }

        template<class F, typename... Args>
        TaskBase(const TaskType type, F&& f, Args&&...args) : _type(type) {
            using return_type = typename std::invoke_result_t<std::decay_t<F>, const CancellationToken&, std::decay_t<Args>...>;
            _function = [this, f = std::forward<F>(f), args...]() {
                TaskState state = TaskState::IDLE;
                if (_state.compare_exchange_strong(state, TaskState::RUN, std::memory_order_release, std::memory_order_relaxed)) {
                    if constexpr (std::is_same_v<return_type, void>) {
                        f(_token, args...);
                    } else {
                        static_cast<Task2<return_type>*>(this)->_result = f(_token, args...);
                    }

                    state = TaskState::RUN;
                    if (_state.compare_exchange_strong(state, TaskState::COMPLETE, std::memory_order_release, std::memory_order_relaxed)) {
                        notify();
                    } else {
                        // task has been cancelled
                    }
                }
            };
        }

        inline explicit operator bool() const noexcept { return _function != nullptr; }
        inline void operator()() const noexcept { _function(); }

        inline TaskType type() const noexcept { return _type; }
    private:
        TaskType _type = TaskType::COMMON;
        Locker _locker;
        CondVar _condition;
        CancellationToken _token;
        std::atomic<TaskState> _state = { TaskState::IDLE };

        std::function<void()> _function = nullptr;
    };

    template <typename T>
    class Task2 : public TaskBase {
        friend class ThreadPool;
        friend class TaskBase;
    public:
        template<class F, typename... Args>
        explicit Task2(F&& f, Args&&...args) : TaskBase(std::forward<F>(f), std::forward<Args>(args)...) { }
    private:
        std::optional<T> _result;
    };

    template <>
    class Task2<void> : public TaskBase {
    public:
        template<class F, typename... Args>
        explicit Task2(F&& f, Args&&...args) : TaskBase(std::forward<F>(f), std::forward<Args>(args)...) { }
    };
}