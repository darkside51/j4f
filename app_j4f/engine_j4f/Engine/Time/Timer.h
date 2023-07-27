#pragma once

#include "Time.h"
#include <cstdint>
#include <functional>

namespace engine {

    enum class TimerState : uint8_t {
        Finish = 0u,
        Continue,
        NotReady
    };

    using TimerExecutor = std::function<TimerState()>;

    class Timer {
        friend class TimerManager;

    public:
        Timer(TimerExecutor &&executor, uint64_t milliseconds) : _period(milliseconds),
                                                                 _fireTime(steadyTime<std::chrono::milliseconds>() +
                                                                           _period),
                                                                 _executor(std::move(executor)) {

        }

        Timer(TimerExecutor &&executor, SteadyTimePoint timePoint) :
                _period(time<SteadyTimePoint, std::chrono::milliseconds>(timePoint) -
                        steadyTime<std::chrono::milliseconds>()),
                _fireTime(steadyTime<std::chrono::milliseconds>() +
                          _period),
                _executor(std::move(executor)) {

        }

        Timer(Timer &&t) noexcept: _executor(std::move(t._executor)) {
            t._executor = nullptr;
        }

        Timer &operator=(Timer &&t) noexcept {
            _executor = std::move(t._executor);
            t._executor = nullptr;
            return *this;
        }

        Timer(const Timer &t) = delete;

        Timer &operator=(const Timer &t) = delete;

        inline TimerState operator()(uint64_t time) {
//            if (_fireTime > time) return TimerState::NotReady;
            const auto result = _executor();
            if (result == TimerState::Continue) {
                _fireTime = time + _period;
            }
            return result;
        }

        inline uint64_t fireTime() const noexcept { return _fireTime; }

    private:
        uint64_t _period;
        uint64_t _fireTime;
        TimerExecutor _executor;
    };

}