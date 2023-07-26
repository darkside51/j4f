#pragma once

#include "../Core/EngineModule.h"
#include "Timer.h"

#include <vector>
#include <mutex>
#include <type_traits>

namespace engine {

    using TimerHandler = const Timer *;

    class TimerManager : public IEngineModule {
        struct NoLock {};
    public:
        template <typename T = NoLock>
        void update(T&& lock) {
            if constexpr (std::is_same_v<std::decay_t<T>, NoLock>) {
                update();
            } else {
                std::lock_guard<T>(lock);
                update();
            }
        }

        template <typename T = NoLock>
        TimerHandler registerTimer(T&& lock, TimerExecutor &&executor, uint64_t milliseconds) {
            if constexpr (std::is_same_v<std::decay_t<T>, NoLock>) {
                return registerTimer(std::move(executor), milliseconds);
            } else {
                std::lock_guard<T>(lock);
                return registerTimer(std::move(executor), milliseconds);
            }
        }

        template <typename T = NoLock>
        void unregisterTimer(T&& lock, TimerHandler h) {
            if constexpr (std::is_same_v<std::decay_t<T>, NoLock>) {
                unregisterTimer(h);
            } else {
                std::lock_guard<T>(lock);
                unregisterTimer(h);
            }
        }

    private:
        TimerHandler registerTimer(TimerExecutor &&executor, uint64_t milliseconds) {
            return &_timers.emplace_back(std::move(executor), milliseconds);
        }

        void unregisterTimer(TimerHandler h) {
            _timers.erase(std::remove_if(_timers.begin(), _timers.end(),
                                         [h](const Timer &t) -> bool { return &t == h; }),
                          _timers.end());
        }
        void update();
        std::vector<Timer> _timers;
    };

}
