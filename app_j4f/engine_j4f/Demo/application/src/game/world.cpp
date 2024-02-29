#include "world.h"
#include "map/map.h"
#include "logic/units/units_manager.h"

#include "service_locator.h"
#include "logic/player_controller.h"

#include "graphics/scene.h"
#include "graphics/ui_manager.h"
#include "ui/main_screen.h"

namespace game {

    World::World() : _map(std::make_unique<Map>()), _unitsManager(std::make_unique<UnitsManager>()) {
        ServiceLocator::instance().getService<Scene>()->getUIManager()->registerUIModule(std::make_unique<MainScreen>());
    }

    World::~World() {

    }

    void World::create() {
        {
            auto &unit = _unitsManager->createUnit("unit_0");
            auto playerController = ServiceLocator::instance().getService<PlayerController>();
            playerController->assign(unit);
        }
        {
            auto &unit = _unitsManager->createUnit("unit_1");
            unit.setPosition(engine::vec3f(30.0f, 0.0f, 0.0f));
            unit.setRotation(engine::vec3f(0.0f, 0.0f, -1.0f));
        }
    }

    UnitsManager& World::getUnitsManager() {
        return *_unitsManager;
    }

    void World::update(const float delta) {
        if (_map) {
            _map->update(delta);
        }

        const auto & camera = ServiceLocator::instance().getService<Scene>()->getCamera(0u);
        _unitsManager->update(delta, camera);
    }

}