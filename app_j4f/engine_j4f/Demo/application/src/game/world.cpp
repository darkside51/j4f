#include "world.h"
#include "map/map.h"
#include "logic/units/units_manager.h"

namespace game {

    World::World() : _map(std::make_unique<Map>()), _unitsManager(std::make_unique<UnitsManager>()) {
        auto & unit = _unitsManager->createUnit();
        unit.setMoveTarget({-1000.0f, 1000.0f, 0.0f});
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