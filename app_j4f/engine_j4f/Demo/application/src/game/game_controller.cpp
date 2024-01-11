#include "game_controller.h"

#include <Engine/Core/Math/mathematic.h>
#include <Engine/Core/Threads/WorkersCommutator.h>
#include <Engine/Graphics/Graphics.h>
#include <Engine/Log/Log.h>

namespace game {
    bool GameController::onInputPointerEvent(const engine::PointerEvent &event) {
        auto &&engineInstance = engine::Engine::getInstance();
        auto &&renderer = engineInstance.getModule<engine::Graphics>().getRenderer();
        const auto [width, height] = renderer->getSize();

        constexpr float kMoveBorderWidth = 20.0f;
        switch (event.state) {
            case engine::InputEventState::IES_NONE: {
                static engine::vec2f currentDirection{0.0f, 0.0f};

                const bool xOnBorder = event.x < kMoveBorderWidth || event.x > width - kMoveBorderWidth;
                const bool yOnBorder = event.y < kMoveBorderWidth || event.y > height - kMoveBorderWidth;

                const float dx = xOnBorder ? event.x - width * 0.5f : 0.0f;
                const float dy = yOnBorder ? event.y - height * 0.5f : 0.0f;

                const engine::vec2f direction{(dx == 0.0f ? 0.0f : (dx > 0.0f ? 1.0f : -1.0f)),
                                              (dy == 0.0f ? 0.0f : (dy > 0.0f ? 1.0f : -1.0f))};

                if (currentDirection != direction) {
                    currentDirection = direction;
                    engineInstance.getModule<engine::WorkerThreadsCommutator>().enqueue(
                            engineInstance.getThreadCommutationId(engine::Engine::Workers::RENDER_THREAD),
                            [direction, this]() {
                                _cameraController.move(direction);
//                                LOG_TAG_LEVEL(engine::LogLevel::L_MESSAGE, GameController, "move camera by %f,%f",
//                                              direction.x, direction.y);
                            });
                    return true;
                }
            }
                break;
            default:
                break;
        }

        return false;
    }

    bool GameController::onInputWheelEvent(const float dx, const float dy) {
        return false;
    }

    bool GameController::onInpuKeyEvent(const engine::KeyEvent &event) {
        return false;
    }

    bool GameController::onInpuCharEvent(const uint16_t code) {
        return false;
    }
}