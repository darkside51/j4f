#include "TimerManager.h"
#include "Time.h"
#include <cstdint>

namespace engine {

    void TimerManager::update() {
        auto const currentTime = steadyTime<std::chrono::milliseconds>();
        _timers.erase(std::remove_if(_timers.begin(), _timers.end(),
                                     [currentTime](Timer &timer) -> bool {
                                         return
                                            (timer.fireTime() <= currentTime &&
                                            timer(currentTime) == TimerState::Finish);
                                     }),
                      _timers.end());
    }

}
