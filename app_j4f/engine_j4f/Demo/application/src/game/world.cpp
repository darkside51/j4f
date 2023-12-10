#include "world.h"
#include "map/map.h"

namespace game {

    World::World(engine::ref_ptr<Scene> scene) : _scene(scene), _map(std::make_unique<Map>(_scene)) {

    }

    World::~World() {

    }

    void World::update(const float delta) {
        if (_map) {
            _map->update(delta);
        }
    }

}