#include "TimerManager.h"
#include "Time.h"

namespace engine {

    void TimerManager::_update() {
        auto const currentTime = steadyTime<std::chrono::milliseconds>();
        _timers.erase(std::remove_if(_timers.begin(), _timers.end(),
                                     [currentTime](Timer &timer) -> bool {
                                         return (timer._fireTime > currentTime) ? false : (timer(currentTime) ==
                                                                                           TimerState::Finish);
                                     }),
                      _timers.end());
    }

}
