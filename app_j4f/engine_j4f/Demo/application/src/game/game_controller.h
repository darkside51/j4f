#pragma once

#include "graphics/camera_controller.h"
#include <Engine/Input/Input.h>

#include <array>

namespace game {
    class GameController {
    public:
        bool onInputPointerEvent(const engine::PointerEvent& event);
        bool onInputWheelEvent(const float dx, const float dy);
        bool onInpuKeyEvent(const engine::KeyEvent& event);
        bool onInpuCharEvent(const uint16_t code);

        CameraController & getCameraController() & { return _cameraController; }
    private:
        bool moveCamera(const engine::PointerEvent& event);
        bool rotateCamera(const engine::PointerEvent& event);
        bool zoomCamera(const float value);

        std::array<bool, 3u> _mouseButtons = { false, false, false };
        CameraController _cameraController;

        engine::vec2f _cameraMoveDirection = engine::vec2f{ 0.0f, 0.0f };
        engine::vec2f _cameraRotation = engine::vec2f{ 0.0f, 0.0f };
    };
}