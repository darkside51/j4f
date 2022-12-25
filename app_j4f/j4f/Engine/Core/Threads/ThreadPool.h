#pragma once

#include "../EngineModule.h"
#include "../../Log/Log.h"
#include "Task.h"

#include <map>
#include <vector>
#include <deque>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace engine {

////////////////////////////////////////////////////////////////////////////////
//
// modified simple C++11 Thread Pool implementation
// https://github.com/progschj/ThreadPool
//
//

    class ThreadPool : public IEngineModule {
        enum class TPoolState : uint8_t {
            RUN = 0,
            PAUSE = 1,
            STOP = 2
        };

    public:
        ThreadPool(uint8_t threads_count) : _threads_count(threads_count), _state(TPoolState::RUN) {
            //log("create ThreadPool with {} threads", threads_count);
            log("create ThreadPool with %d threads", threads_count);
            _current_worker_tasks.resize(_threads_count);
            _workers.reserve(_threads_count);
            for (uint8_t i = 0; i < _threads_count; ++i) {
                _workers.emplace_back(&ThreadPool::threadFunction, this, i);
            }
        }

        ~ThreadPool() {
            stop();
        }

        void stop() {
            {
                std::unique_lock<std::mutex> lock(_queue_mutex);
                _state = TPoolState::STOP;
            }

            cancelTasks(0b11111111); // stop вызовет отмену всех задач, независимо от типа
            _condition.notify_all();

            for (std::thread& worker : _workers) { worker.join(); }
            _workers.clear();
        }
        
        void pause(const uint8_t typeMask = 0b00000001) {
            if (const bool paused = _state == TPoolState::RUN) {
                {
                    std::unique_lock<std::mutex> lock(_queue_mutex);
                    _state = TPoolState::PAUSE;
                }

                cancelTasks(typeMask); // отменит задачи, по типам, согласно маске typeMask
                _condition.notify_all();

                LOG_TAG_LEVEL(engine::LogLevel::L_CUSTOM, THREADPOOL, "ThreadPool paused");
            }
        }

        void resume() {
            if (const bool resumed = _state == TPoolState::PAUSE) {
                {
                    std::unique_lock<std::mutex> lock(_queue_mutex);
                    _state = TPoolState::RUN;
                }

                _condition.notify_all();

                LOG_TAG_LEVEL(engine::LogLevel::L_CUSTOM, THREADPOOL, "ThreadPool resumed");
            }
        }

        void cancelTasks(const uint8_t typeMask) {
            for (auto it = _tasks_map.begin(); it != _tasks_map.end(); ++it) {
                for (ITaskHandlerPtr& task : it->second) {
                    const uint8_t taskType = static_cast<uint8_t>(task->_type);
                    if (typeMask & (1 << taskType)) {
                        task->cancel();
                    }
                }
            }

            for (auto&& task : _current_worker_tasks) {
                if (task) {
                    const uint8_t taskType = static_cast<uint8_t>(task->_type);
                    if (typeMask & (1 << taskType)) {
                        task->cancel();
                    }
                }
            }
        }

        /////////////////// raw task enqueue
        /*template<class F> auto enqueue_raw(const TaskType type, const uint16_t priority, F&& f)->TaskResult<typename std::invoke_result_t<std::decay_t<F>>> {
            using return_type = typename std::invoke_result_t<std::decay_t<F>>;
            TaskResult<return_type> result = makeRawTaskResult(type, std::forward<F>(f));
            enqueue(priority, result);
            return result;
        }

        template<class F, class... Args> auto enqueue_raw(const TaskType type, const uint16_t priority, F&& f, Args&&... args)->TaskResult<typename std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> {
            using return_type = typename std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;
            TaskResult<return_type> result = makeRawTaskResult(type, std::forward<F>(f), std::forward<Args>(args)...);
            enqueue(priority, result);
            return result;
        }*/

        /////////////////// task enqueue
        template<class F> auto enqueue(const TaskType type, const uint16_t priority, F&& f)->TaskResult<typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>>> {
            using return_type = typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>>;
            TaskResult<return_type> result = makeTaskResult(type, std::forward<F>(f));
            enqueue(priority, result);
            return result;
        }

        template<class F, class... Args> auto enqueue(const TaskType type, const uint16_t priority, F&& f, Args&&... args)->TaskResult<typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>, std::decay_t<Args>...>> {
            using return_type = typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>, std::decay_t<Args>...>;
            TaskResult<return_type> result = makeTaskResult(type, std::forward<F>(f), std::forward<Args>(args)...);
            enqueue(priority, result);
            return result;
        }

        /////////////////// taskResult enqueue
        template<class T> inline void enqueue(const uint16_t priority, TaskResult<T>& result) {
            if (result.state() == TaskState::CANCELED) return;

            {
                std::unique_lock<std::mutex> lock(_queue_mutex);

                // don't allow enqueueing after stopping the pool
                if (_state != TPoolState::RUN) {
                    printf("enqueue on stopped ThreadPool, !!!!!invalidate result!!!!!\n");
                    result.invalidate();
                    return;
                }

                _tasks_map[priority].emplace_back(result._handler);
            }

            _condition.notify_one();
        }

        inline void enqueue(const uint16_t priority, ITaskHandlerPtr& handler) {
            if (handler->state() == TaskState::CANCELED) return;

            {
                std::unique_lock<std::mutex> lock(_queue_mutex);

                // don't allow enqueueing after stopping the pool
                if (_state != TPoolState::RUN) {
                    printf("enqueue on stopped ThreadPool, !!!!!invalidate result!!!!!\n");
                    handler->invalidate(); // todo what do with future from TaskResult?
                    return;
                }

                _tasks_map[priority].emplace_back(handler);
            }

            _condition.notify_one();
        }

        inline uint8_t capacity() const { return _threads_count; }
        inline TPoolState state() const { return _state; }

    private:
        inline void threadFunction(const uint8_t threadId) {
            for ( ; ; ) {
                ITaskHandlerPtr& task = _current_worker_tasks[threadId];
                {
                    std::unique_lock<std::mutex> lock(_queue_mutex);
                    _condition.wait(lock, [this] { 
                        return (_state == TPoolState::STOP) || !_tasks_map.empty();
                    });

                    switch (_state) {
                        case TPoolState::PAUSE:
                            if (_tasks_map.empty()) { continue; }
                            break;
                        case TPoolState::STOP:
                            if (_tasks_map.empty()) { return; }
                            break;
                        default:
                            break;
                    }

                    auto it = --_tasks_map.end(); // last element of map

                    std::deque<ITaskHandlerPtr>& queue = it->second;

                    while (!queue.empty() && (queue.front()->state() != TaskState::IDLE)) { // immidiate remove cancelled tasks
                        queue.pop_front();
                    }

                    if (queue.empty()) { 
                        _tasks_map.erase(it);
                    } else {
                        task = std::move(queue.front());
                        queue.pop_front();

                        if (queue.empty()) { _tasks_map.erase(it); }
                    }
                }

                if (task) {
                    task->operator()();
                    task = nullptr;
                }
            }
        }

        // need to keep track of threads so we can join them
        std::vector<std::thread> _workers;
        std::vector<ITaskHandlerPtr> _current_worker_tasks;
        // the task queue
        std::map<uint16_t, std::deque<ITaskHandlerPtr>> _tasks_map;

        // synchronization
        std::mutex _queue_mutex;
        std::condition_variable _condition;
        uint8_t _threads_count;
        TPoolState _state;
    };
}