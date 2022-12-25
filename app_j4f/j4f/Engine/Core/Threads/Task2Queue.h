#pragma once

#include "../Linked_ptr.h"
#include "Task2.h"
#include <deque>

namespace engine {

    enum class ThreadQueueState : uint8_t {
        RUN = 0,
        PAUSE = 1,
        STOP = 2
    };

    using Locker = SpinLock;
    using CondVar = std::condition_variable_any;

    class Task2Queue {
        friend class ThreadPool2;
    public:
        template <typename T>
        void enqueue(const linked_ptr<T>& task) noexcept {
            {
                std::lock_guard<Locker> guard(_locker);
                if (_state == ThreadQueueState::STOP) {
                    return;
                }

                _tasks.emplace_back(std::static_pointer_cast<TaskBase>(task));
            }
            _condition.notify_one();
        }

        inline void stop() noexcept {
            {
                std::lock_guard<Locker> guard(_locker);
                _state = ThreadQueueState::STOP;
            }
        }

        inline void pause() noexcept {
            {
                std::lock_guard<Locker> guard(_locker);
                _state = ThreadQueueState::PAUSE;
            }
        }

        inline void resume() noexcept {
            {
                std::lock_guard<Locker> guard(_locker);
                _state = ThreadQueueState::RUN;
            }
        }

        inline void notify() noexcept {
            _condition.notify_all();
        }

        inline void cancelTasks(const uint8_t typeMask) {
            std::unique_lock<Locker> lock(_locker);
            for (auto&& t : _tasks) {
                if (typeMask & (1 << static_cast<uint8_t>(t->_type))) {
                    t->cancel();
                }
            }
            _tasks.clear();
        }

    private:
        ThreadQueueState _state = ThreadQueueState::RUN;
        Locker _locker;
        CondVar _condition;
        std::deque<linked_ptr<TaskBase>> _tasks;
    };
}