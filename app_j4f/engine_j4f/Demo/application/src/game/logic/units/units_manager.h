#pragma once

#include "unit.h"

#include <Engine/Core/ref_ptr.h>

#include <memory>
#include <vector>

namespace engine {
    struct MeshGraphicsDataBuffer;
}

namespace game {
    class UnitsManager {
    public:
        UnitsManager();
        ~UnitsManager();

        Unit& createUnit();
        void removeUnit();

        void update(const float delta);

        engine::ref_ptr<engine::MeshGraphicsDataBuffer> getGraphicsBuffer();

    private:
        std::unique_ptr<engine::MeshGraphicsDataBuffer> _meshGraphicsBuffer;
        std::vector<Unit> _units;
    };
}