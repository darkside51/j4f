#pragma once

#include "unit.h"
#include <string_view>
#include <vector>

namespace engine {
    class Camera;
}

namespace game {
    class UnitsManager {
    public:
        UnitsManager();
        ~UnitsManager();

        Unit& createUnit(std::string_view name);
        void removeUnit();

        void update(const float delta, const engine::Camera& cam);

    private:
        std::vector<Unit> _units;
    };
}