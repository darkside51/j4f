#include "world.h"
#include "map/map.h"
#include "logic/units/units_manager.h"

#include "service_locator.h"
#include "logic/player_controller.h"

namespace game {

    World::World() : _map(std::make_unique<Map>()), _unitsManager(std::make_unique<UnitsManager>()) {
        auto & unit = _unitsManager->createUnit();
        auto playerController = ServiceLocator::instance().getService<PlayerController>();
        playerController->assign(unit);
    }

    World::~World() {

    }

    UnitsManager& World::getUnitsManager() {
        return *_unitsManager;
    }

    void World::update(const float delta) {
        if (_map) {
            _map->update(delta);
        }

        _unitsManager->update(delta);
    }

}