#pragma once

#include <Engine/Core/ref_ptr.h>
#include <memory>

namespace game {
    class Scene;
    class Map;

    class World {
    public:
        World();
        ~World();

        void update(const float delta);

    private:
        std::unique_ptr<Map> _map;
    };

}