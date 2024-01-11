#pragma once

#include <Engine/Core/Ref_ptr.h>
#include <Engine/Core/Math/mathematic.h>

namespace engine {
    class Camera;
}

namespace game {
    class CameraController {
    public:
        CameraController() = default;

        void assignCamera(engine::ref_ptr<engine::Camera> camera) noexcept { _camera = camera; }

        void setPosition(const engine::vec3f & position) noexcept;
        void move(const engine::vec2f & direction) noexcept;
        const engine::vec3f & getPosition() const noexcept { return _position; }

        bool update(const float delta);

    private:
        bool _dirty = true;
        engine::vec3f _position = engine::vec3f(0.0f);
        engine::vec2f _moveDirection = engine::vec2f(0.0f);
        engine::ref_ptr<engine::Camera> _camera;
    };
}