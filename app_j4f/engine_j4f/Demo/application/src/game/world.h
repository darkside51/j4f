#pragma once

#include <Engine/Core/Ref_ptr.h>
#include <memory>

namespace game {
    class Scene;
    class Map;

    class World {
    public:
        World(engine::ref_ptr<Scene> scene);
        ~World();

        void update(const float delta);

    private:
        engine::ref_ptr<Scene> _scene;
        std::unique_ptr<Map> _map;
    };

}