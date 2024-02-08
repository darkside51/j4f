#pragma once

#include "unit.h"

#include <vector>

namespace game {
    class UnitsManager {
    public:
        void update(const float delta);

    private:
        std::vector<Unit> _units;
    };
}