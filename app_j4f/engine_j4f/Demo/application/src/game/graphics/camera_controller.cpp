#include "camera_controller.h"

#include <Engine/Graphics/Camera.h>

namespace game {
    constexpr float kEps = 1e-5f;
    constexpr float kCameraMoveSpeed = 500.0f;
    constexpr float kMaxPitch = 0.0f; // in radians
    constexpr float kMinPitch = -1.125f; // in radians
    constexpr float kMinLen = 100.0f;
    constexpr float kMaxLen = 500.0f;

    void CameraController::setPosition(const engine::vec3f & position) noexcept {
        if (engine::compare(position, _position, kEps)) {
            _position = position;
            _dirty = true;
        }
    }

    void CameraController::move(const engine::vec2f & direction) noexcept {
        if (direction.x == 0.0f && direction.y == 0.0f) {
            _moveDirection = { 0.0f, 0.0f };
            return;
        }

        const auto size = _camera->getSize();
        const float aspect = size.x / size.y;
        
        auto const dir = _camera->getInvViewTransform() * engine::vec4f(direction.x, direction.y * aspect, 0.0f, 0.0f);
        _moveDirection = engine::as_normalized(engine::vec2f(dir.x, dir.y));
    }

    void CameraController::addPitchYaw(const engine::vec2f& py) noexcept {
        if (py.x > kEps || py.x < -kEps || py.y > kEps || py.y < -kEps) {
            _pitchYaw += py;
            _pitchYaw.x = std::clamp(_pitchYaw.x, kMinPitch, kMaxPitch);
            _dirty = true;
        }
    }

    void CameraController::addLen(const float l) noexcept {
        if (l > kEps || l < -kEps) {
            _len += l;
            _len = std::clamp(_len, kMinLen, kMaxLen);
            _dirty = true;
        }
    }

    void CameraController::resize(const uint16_t w, const uint16_t h) noexcept {
        auto && size = _camera->getSize();
        if (size.x != w || size.y != h) {
            _camera->resize(w, h);
            _dirty = true;
        }
    }

    bool CameraController::update(const float delta) {
        if ((!_dirty && _moveDirection == engine::vec2f{0.0f, 0.0f}) || _camera == nullptr) return false;
        _dirty = false;

        _position.x += _moveDirection.x * delta * kCameraMoveSpeed;
        _position.y += _moveDirection.y * delta * kCameraMoveSpeed;

        _camera->setRotation(engine::vec3f(_pitchYaw.x, 0.0f, _pitchYaw.y));

        const float rx = _pitchYaw.x;
        const float rz = _pitchYaw.y;
        if (rx != 0.0f || rz != 0.0f) {
            // use spherical coordinates
            const float crx = cosf(rx);
            const float crz = cosf(rz);
            const float srx = sinf(rx);
            const float srz = sinf(rz);

            const float x = _len * srx * srz;
            const float y = _len * srx * crz;
            const float z = _len * crx;

            _camera->setPosition({ _position.x + x, _position.y + y, _position.z + z });
        } else {
            _camera->setPosition(_position);
        }

        return _camera->calculateTransform();
    }

}