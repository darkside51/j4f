#include "game_controller.h"

namespace game {
    bool GameController::onInputPointerEvent(const engine::PointerEvent& event) {
        return false;
    }

    bool GameController::onInputWheelEvent(const float dx, const float dy) {
        return false;
    }

    bool GameController::onInpuKeyEvent(const engine::KeyEvent& event) {
        return false;
    }

    bool GameController::onInpuCharEvent(const uint16_t code) {
        return false;
    }
}