#pragma once

#include "../EngineModule.h"
#include "../../Log/Log.h"
#include "Task2Queue.h"
#include <thread>

namespace engine {

    class ThreadPool2 : public IEngineModule {
        enum class TPoolState : uint8_t {
            RUN = 0,
            PAUSE = 1,
            STOP = 2
        };
    public:
        explicit ThreadPool2(const size_t threads_count) : _state(TPoolState::RUN), _threads_count(threads_count), _queues(threads_count), _currentTasks(threads_count) {
            _workers.reserve(_threads_count);
            for (size_t i = 0; i < _threads_count; ++i) {
                _workers.emplace_back(&ThreadPool2::threadFunction, this, i);
            }
        }

        ~ThreadPool2() override {
            stop();
        }

        inline void stop() {
            if (_state.load(std::memory_order_acquire) != TPoolState::STOP) {
                _state.store(TPoolState::STOP, std::memory_order_release);
                for (auto& q : _queues) {
                    q.stop();
                }

                cancelTasks(0b11111111); // cancelTasks ��� ���� �����, ���������� �� ����

                for (auto& q : _queues) {
                    q.notify();
                }

                for (auto& w : _workers) {
                    w.join();
                }
                _workers.clear();

                LOG_TAG_LEVEL(engine::LogLevel::L_CUSTOM, THREADPOOL2, "ThreadPool stopped");
            }
        }

        inline void pause(const uint8_t typeMask = 0b00000001) {
            TPoolState runState = TPoolState::RUN;
            if (_state.compare_exchange_strong(runState, TPoolState::PAUSE, std::memory_order_release, std::memory_order_relaxed)) {
                for (auto& q : _queues) {
                    q.pause();
                }

                cancelTasks(typeMask);

                for (auto& q : _queues) {
                    q.notify();
                }

                LOG_TAG_LEVEL(engine::LogLevel::L_CUSTOM, THREADPOOL2, "ThreadPool paused");
            }
        }

        inline void resume() {
            TPoolState pauseState = TPoolState::PAUSE;
            if (_state.compare_exchange_strong(pauseState, TPoolState::RUN, std::memory_order_release, std::memory_order_relaxed)) {
                for (auto& q : _queues) {
                    q.resume();
                    q.notify();
                }

                LOG_TAG_LEVEL(engine::LogLevel::L_CUSTOM, THREADPOOL2, "ThreadPool resumed");
            }
        }

        template<class F, typename... Args>
        auto enqueue(const TaskType type, F&& f, Args&&... args) -> linked_ptr<Task2<typename std::invoke_result_t<std::decay_t<F>, const CancellationToken&, std::decay_t<Args>...>>> {
            const uint8_t idx = _taskIdx.fetch_add(1, std::memory_order_release) % _threads_count;
            using return_type = typename std::invoke_result_t<std::decay_t<F>, const CancellationToken&, std::decay_t<Args>...>;
            auto task = make_linked<Task2<return_type>>(type, std::forward<F>(f), std::forward<Args>(args)...);
            _queues[idx].enqueue(task);

            return task;
        }

        inline void cancelTasks(const uint8_t typeMask) {
            for (auto&& t : _currentTasks) {
                if (t && (typeMask & (1 << static_cast<uint8_t>(t->_type)))) {
                    t->cancel();
                }
            }

            for (auto&& q : _queues) {
                q.cancelTasks(typeMask);
            }
        }

    private:
        inline void grabTask(linked_ptr<TaskBase>& task, const uint8_t threadId) {
            for (size_t i = 0; i < _threads_count; ++i) {
                if (i == threadId) continue;
                auto& current = _queues[i];

                while (!current._tasks.empty() && (current._tasks.front()->state() != TaskState::IDLE)) { // immediate remove cancelled tasks
                    current._tasks.pop_front();
                }

                if (!current._tasks.empty()) {
                    std::unique_lock<Locker> lock(current._locker);
                    if (!current._tasks.empty()) {
                        task = std::move(current._tasks.front());
                        current._tasks.pop_front();
                        return;
                    }
                }
            }
        }

        inline void threadFunction(const uint8_t threadId) {
            for ( ; ; ) {
                auto& task = _currentTasks[threadId];
                {
                    auto& current = _queues[threadId];
                    std::unique_lock<Locker> lock(current._locker);
                    current._condition.wait(lock, [this, threadId] { return (_queues[threadId]._state == ThreadQueueState::STOP) || (!_queues[threadId]._tasks.empty()); });

                    while (!current._tasks.empty() && (current._tasks.front()->state() != TaskState::IDLE)) { // immediate remove cancelled tasks
                        current._tasks.pop_front();
                    }

                    if (current._tasks.empty()) { // try to get task from other queue
                        lock.unlock();
                        grabTask(task, threadId);
                        switch (current._state) {
                            case ThreadQueueState::PAUSE:
                                if (!task) {
                                    continue;
                                }
                                break;
                            case ThreadQueueState::STOP:
                                if (!task) {
                                    return;
                                }
                                break;
                            default:
                                break;
                        }
                    } else {
                        task = std::move(current._tasks.front());
                        current._tasks.pop_front();
                    }
                }

                if (task) {
                    task->operator()();
                    task = nullptr;
                }
            }
        }

        std::atomic<TPoolState> _state;
        size_t _threads_count;
        std::atomic_uint16_t _taskIdx = 0u;
        std::vector<std::thread> _workers;
        std::vector<Task2Queue<SpinLock, std::condition_variable_any>> _queues;
        std::vector<linked_ptr<TaskBase>> _currentTasks;
    };
}