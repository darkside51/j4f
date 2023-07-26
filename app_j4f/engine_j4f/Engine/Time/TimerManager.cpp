#include "TimerManager.h"
#include "Time.h"
#include <cstdint>

namespace engine {

    void TimerManager::update() {
        auto const currentTime = steadyTime<std::chrono::milliseconds>();

        uint32_t i = 0u;
        const uint32_t sz = _timers.size();
        std::vector<Timer *> _toRemove;
        _toRemove.reserve(sz);
        for (auto &&timer: _timers) {
            if (timer.fireTime() <= currentTime) {
                if (timer(currentTime) == TimerState::Finish) {
                    _toRemove.push_back(&timer);
                }
            }
            ++i;
        }

        if (_toRemove.empty()) return;

        _timers.erase(std::remove_if(_timers.begin(), _timers.end(),
                                     [&_toRemove](const Timer &t) -> bool {
                                         return std::find(_toRemove.begin(), _toRemove.end(), &t) != _toRemove.end();
                                     }),
                      _timers.end());
    }

}
