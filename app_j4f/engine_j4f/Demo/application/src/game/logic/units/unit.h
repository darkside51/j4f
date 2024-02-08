#pragma once

#include <Engine/Core/Math/mathematic.h>
#include <Engine/Core/ref_ptr.h>
#include "../entity.h"
#include "../../map/object.h"

#include <memory>

namespace engine {
    class MeshAnimationTree;
}

namespace game {

    enum class UnitState : uint8_t {
        Undefined = 0u,
        Idle = 1u,
        Walking = 2u,
        Running = 3u,
    };

    class Unit : public Entity {
    public:
        Unit();
        ~Unit();

        void update(const float delta);

    private:
        UnitState _state = UnitState::Undefined;
        MapObject _mapObject;
        std::unique_ptr<engine::MeshAnimationTree> _animations;
        engine::vec3f _moveTarget = {0.0f, 0.0f, 0.0f};
        engine::ref_ptr<Entity> _target = nullptr;
    };

}