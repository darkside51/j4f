#pragma once

#include "graphics/camera_controller.h"
#include <Engine/Input/Input.h>

namespace game {
    class GameController {
    public:
        bool onInputPointerEvent(const engine::PointerEvent& event);
        bool onInputWheelEvent(const float dx, const float dy);
        bool onInpuKeyEvent(const engine::KeyEvent& event);
        bool onInpuCharEvent(const uint16_t code);

        CameraController & getCameraController() & { return _cameraController; }
    private:
        CameraController _cameraController;
    };
}