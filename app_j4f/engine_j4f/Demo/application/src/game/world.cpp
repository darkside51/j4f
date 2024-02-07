#include "world.h"
#include "map/map.h"

namespace game {

    World::World() : _map(std::make_unique<Map>()) {

    }

    World::~World() {

    }

    void World::update(const float delta) {
        if (_map) {
            _map->update(delta);
        }
    }

}