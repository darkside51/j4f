#include "game_controller.h"

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

                if (event.x < kMoveBorderWidth || event.x > width - kMoveBorderWidth || event.y < kMoveBorderWidth ||
                    event.y > height - kMoveBorderWidth) {
                    engineInstance.getModule<engine::WorkerThreadsCommutator>().enqueue(
                            engineInstance.getThreadCommutationId(engine::Engine::Workers::UPDATE_THREAD),
                            [x = event.x, y = event.y, width, height]() {
                                if (x < kMoveBorderWidth || x > width - kMoveBorderWidth) {
                                    LOG_TAG_LEVEL(engine::LogLevel::L_MESSAGE, GameController, "move camera by ox");
                                }

                                if (y < kMoveBorderWidth || y > height - kMoveBorderWidth) {
                                    LOG_TAG_LEVEL(engine::LogLevel::L_MESSAGE, GameController, "move camera by oy");
                                }
                            });
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