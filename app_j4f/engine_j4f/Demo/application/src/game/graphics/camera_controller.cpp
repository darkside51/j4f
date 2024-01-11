#include "camera_controller.h"

#include <Engine/Graphics/Camera.h>

namespace game {
    constexpr float kEps = 1e-5f;

    void CameraController::setPosition(const engine::vec3f & position) noexcept {
        if (engine::compare(position, _position, kEps)) {
            _position = position;
            _dirty = true;
        }
    }

    void CameraController::move(const engine::vec2f & direction) noexcept {
        _moveDirection = direction;
    }

    bool CameraController::update(const float delta) {
        if ((!_dirty && _moveDirection == engine::vec2f{0.0f, 0.0f}) || _camera == nullptr) return false;
        _dirty = false;

        constexpr float cameraMoveSpeed = 500.0f;

        _position.x += _moveDirection.x * delta * cameraMoveSpeed;
        _position.y += _moveDirection.y * delta * cameraMoveSpeed;

        _camera->setPosition(_position);
        return _camera->calculateTransform();
    }

}