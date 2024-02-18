#pragma once

#include <Engine/Core/Math/mathematic.h>
#include <Engine/Core/ref_ptr.h>
#include <Engine/Graphics/Mesh/AnimationTree.h>
#include "../entity.h"
#include "../../map/object.h"

#include <cstdint>
#include <memory>

namespace game {

    enum class UnitState : uint8_t {
        Undefined = 0u,
        Idle = 1u,
        Walking = 2u,
        Running = 3u,
        Special = 4u,
    };

class Unit : public Entity, public engine::IAnimationObserver {
    public:
        Unit();
        ~Unit();

        Unit(Unit &&) noexcept;

        void onEvent(engine::AnimationEvent event, const engine::MeshAnimator* animator) override;

        void setMoveTarget(const engine::vec3f & t) { _moveTarget = t; }
        void update(const float delta);

        engine::vec3f getPosition() const;
        void setState(const UnitState state, uint8_t specialAnimId = 0u) noexcept;

    private:
        void updateAnimationState(const float delta);

        uint8_t _currentAnimId = 0u;
        UnitState _state = UnitState::Undefined;
        MapObject _mapObject;
        std::unique_ptr<engine::MeshAnimationTree> _animations;
        engine::vec3f _moveTarget = {0.0f, 0.0f, 0.0f};
        engine::vec3f _direction = {0.0f, -1.0f, 0.0f};
        engine::ref_ptr<Entity> _target = nullptr;
    };

}