#include "game_controller.h"

#include <Engine/Core/Engine.h>
#include <Engine/Core/Math/mathematic.h>
#include <Engine/Core/Threads/WorkersCommutator.h>
#include <Engine/Graphics/Graphics.h>
#include <Engine/Log/Log.h>

namespace game {
    constexpr float kMoveBorderWidth = 4.0f;
    constexpr float kCameraRotationSpeed = 0.005f;
    constexpr float kCameraZoomSpeed = 20.0f;
    constexpr bool kFollowPlayerMode = true;

    bool GameController::moveCamera(const engine::PointerEvent& event) {
        if constexpr (kFollowPlayerMode) return false;

        auto&& engineInstance = engine::Engine::getInstance();
        auto&& renderer = engineInstance.getModule<engine::Graphics>().getRenderer();
        const auto [width, height] = renderer->getSize();

        const bool xOnBorder = event.x < kMoveBorderWidth || event.x > width - kMoveBorderWidth;
        const bool yOnBorder = event.y < kMoveBorderWidth || event.y > height - kMoveBorderWidth;
        const bool onBorder = xOnBorder || yOnBorder;

        const float dx = onBorder ? event.x - width * 0.5f : 0.0f;
        const float dy = onBorder ? event.y - height * 0.5f : 0.0f;

        const engine::vec2f direction{ dx, dy };

        if (_cameraMoveDirection != direction) {
            _cameraMoveDirection = direction;
            engineInstance.getModule<engine::WorkerThreadsCommutator>().enqueue(
                engineInstance.getThreadCommutationId(engine::Engine::Workers::RENDER_THREAD),
                [direction, this]() {
                    _cameraController.move(direction);
                });
            return true;
        }
    }

    bool GameController::rotateCamera(const engine::PointerEvent& event) {
        auto&& engineInstance = engine::Engine::getInstance();
        engineInstance.getModule<engine::WorkerThreadsCommutator>().enqueue(
            engineInstance.getThreadCommutationId(engine::Engine::Workers::RENDER_THREAD),
            [this, x = event.x, y = event.y]() {
                _cameraController.addPitchYaw(engine::vec2f{(_cameraRotation.y - y) * kCameraRotationSpeed, -(_cameraRotation.x - x) * kCameraRotationSpeed });
                _cameraRotation = { x, y };
            });
        return true;
    }

    bool GameController::zoomCamera(const float value) {
        if (value == 0.0f) return false;
        auto&& engineInstance = engine::Engine::getInstance();
        engineInstance.getModule<engine::WorkerThreadsCommutator>().enqueue(
            engineInstance.getThreadCommutationId(engine::Engine::Workers::RENDER_THREAD),
            [this, value]() {
                _cameraController.addLen(value);

            });
        return true;
    }

    bool GameController::onInputPointerEvent(const engine::PointerEvent &event) {
        auto &&engineInstance = engine::Engine::getInstance();
        
        switch (event.state) {
        case engine::InputEventState::IES_NONE: {
            if (_mouseButtons[1u]) {
                return rotateCamera(event);
            }

            return moveCamera(event);
        }
                                              break;
        case engine::InputEventState::IES_PRESS:
            _cameraRotation = {event.x, event.y};
            [[fallthrough]];
            case engine::InputEventState::IES_RELEASE:
            {
                const uint8_t buttonId = static_cast<uint8_t>(event.button);
                if (buttonId < _mouseButtons.size()) {
                    _mouseButtons[buttonId] = !_mouseButtons[buttonId];
                }
            }
                break;
            default:
                break;
        }

        _playerController.onPointerEvent(event);

        return false;
    }

    bool GameController::onInputWheelEvent(const float dx, const float dy) {
        return zoomCamera(-dy * kCameraZoomSpeed);
    }

    bool GameController::onInpuKeyEvent(const engine::KeyEvent &event) {
        return _playerController.onKeyEvent(event);
    }

    bool GameController::onInpuCharEvent(const uint16_t code) {
        return false;
    }

    void GameController::onRenderFrame() {
        if constexpr (!kFollowPlayerMode) return;

        constexpr float kHeight = 25.0f;
        _cameraController.setPosition(_playerController.getPlayerPosition() + engine::vec3f{ 0.0f, 0.0f, kHeight });
    }
}