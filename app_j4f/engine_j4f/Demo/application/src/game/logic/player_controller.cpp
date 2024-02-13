#include "player_controller.h"

#include "../service_locator.h"
#include "../graphics/scene.h"
#include "units/unit.h"

#include <Engine/Core/Math/functions.h>
#include <Engine/Core/Threads/WorkersCommutator.h>

namespace game {

    void PlayerController::assign(const Unit &unit) {
        _unit = engine::make_ref(const_cast<Unit&>(unit));
    }

    void PlayerController::onPointerEvent(const engine::PointerEvent &event) {
        if (!_unit) return;

        switch (event.state) {
            case engine::InputEventState::IES_PRESS:
                break;
            case engine::InputEventState::IES_RELEASE:
            {
                const uint8_t buttonId = static_cast<uint8_t>(event.button);
                if (buttonId == 0u) { // left mouse button
                    auto scene = ServiceLocator::instance().getService<Scene>();
                    auto & mainCamera = scene->getCamera(0u);
                    const auto ray = mainCamera.screenToWorld({event.x, event.y});
                    const auto point = engine::intersectionLinePlane(ray.s, ray.f,{0.0f, 0.0f, 1.0f, 0.0f});

                    auto&& engineInstance = engine::Engine::getInstance();
                    engineInstance.getModule<engine::WorkerThreadsCommutator>().enqueue(
                            engineInstance.getThreadCommutationId(engine::Engine::Workers::UPDATE_THREAD),
                            [this, point]() {
                                _unit->setMoveTarget(point);
                            });
                }
            }
                break;
            default:
                break;
        }
    }
}