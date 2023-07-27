#pragma once

#include "../Core/EngineModule.h"
#include "Timer.h"

#include <vector>
#include <mutex>
#include <type_traits>

namespace engine {

    using TimerHandle = const Timer *;

    class TimerManager final : public IEngineModule {
        struct NoLock {};
    public:
        template <typename T = NoLock>
        void update(T&& lock) {
            if constexpr (std::is_same_v<std::decay_t<T>, NoLock>) {
                _update();
            } else {
                std::lock_guard<T> l(lock);
                _update();
            }
        }

        template <typename T = NoLock, typename... Args>
        TimerHandle registerTimer(T&& lock, Args&&... args) {
            if constexpr (std::is_same_v<std::decay_t<T>, NoLock>) {
                return _register(std::forward<Args>(args)...);
            } else {
                std::lock_guard<T> l(lock);
                return _register(std::forward<Args>(args)...);
            }
        }

        template <typename T = NoLock>
        void unregisterTimer(T&& lock, TimerHandle h) {
            if constexpr (std::is_same_v<std::decay_t<T>, NoLock>) {
                _unregister(h);
            } else {
                std::lock_guard<T> l(lock);
                _unregister(h);
            }
        }

    private:
        template <typename... Args>
        inline TimerHandle _register(Args&&... args) {
            return &_timers.emplace_back(std::forward<Args>(args)...);
        }

        inline void _unregister(TimerHandle h) {
            _timers.erase(std::remove_if(_timers.begin(), _timers.end(),
                                         [h](const Timer &t) -> bool { return &t == h; }),
                          _timers.end());
        }
        void _update();
        std::vector<Timer> _timers;
    };

}
