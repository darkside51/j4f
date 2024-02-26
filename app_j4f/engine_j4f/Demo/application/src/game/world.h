#pragma once

#include <Engine/Core/ref_ptr.h>
#include <memory>

namespace game {
    class Scene;
    class Map;
    class UnitsManager;

    class World {
    public:
        World();
        ~World();

        UnitsManager& getUnitsManager();

        void create();

        void update(const float delta);

    private:
        std::unique_ptr<Map> _map;
        std::unique_ptr<UnitsManager> _unitsManager;
    };

}