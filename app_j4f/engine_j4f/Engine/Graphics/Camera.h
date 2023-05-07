#pragma once

#include "../../Core/Math/mathematic.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace engine {

	class Camera;

	struct Ray {
		vec3f s;
		vec3f f;
		Ray(
			const float sx, const float sy, const float sz,
			const float fx, const float fy, const float fz
		) : s(sx, sy, sz), f(fx, fy, fz) {}
	};

	class Frustum {
	public:
		void calculate(const mat4f& clip) noexcept;
		
		bool isPointVisible(const vec3f& p) const noexcept;
		bool isSphereVisible(const vec3f& p, const float r) noexcept;
		bool isCubeVisible_classic(const vec3f& min, const vec3f& max) const noexcept;
		bool isCubeVisible(const vec3f& min, const vec3f& max) const noexcept;

	private:
		void normalize() noexcept;
		bool _normalized;
		float _frustum[6][4];
	};

	class FrustumCollection {
	public:
		FrustumCollection(const uint8_t count) : _frustums(count) {}

		inline void calculate(const std::vector<mat4f>& clips) noexcept {
			size_t i = 0;
			for (auto&& f : _frustums) {
				f.calculate(clips[i++]);
			}
		}

		void calculateOne(const mat4f& clip, const uint8_t idx) noexcept {
			_frustums[idx].calculate(clip);
		}

		inline bool isPointVisible(const vec3f& p) const noexcept {
			for (auto&& f : _frustums) { if (f.isPointVisible(p)) return true; }
		}

		inline bool isSphereVisible(const vec3f& p, const float r) noexcept {
			for (auto&& f : _frustums) { if (f.isSphereVisible(p, r)) return true; }
		}

		inline bool isCubeVisible_classic(const vec3f& min, const vec3f& max) const noexcept {
			for (auto&& f : _frustums) { if (f.isCubeVisible_classic(min, max)) return true; }
		}

		inline bool isCubeVisible(const vec3f& min, const vec3f& max) const noexcept {
			for (auto&& f : _frustums) { if (f.isCubeVisible(min, max)) return true; }
		}

	private:
		std::vector<Frustum> _frustums;
	};

	enum class ProjectionType {
		PERSPECTIVE = 0,
		ORTHO = 1,
		CUSTOM = 2
	};

	class ICameraTransformChangeObserver {
	public:
		virtual ~ICameraTransformChangeObserver() = default;
		virtual void onCameraTransformChanged(const Camera* camera) = 0;
	};

	class Camera {
		struct DirtyFlags {
			bool transform : 1;
			bool invTransform : 1;
			bool invViewTransform : 1;
		};

		union DirtyValue {
			DirtyFlags flags;
			uint8_t v = 0;

			DirtyValue(const uint8_t value) : v(value) {}

			DirtyFlags* operator->() noexcept { return &flags; }
			const DirtyFlags* operator->() const noexcept { return &flags; }
		};

	public:
		Camera(const uint32_t w, const uint32_t h);
		Camera(const vec2f& sz);

		~Camera() noexcept {
			disableFrustum();
		}

		void resize(const float w, const float h) noexcept;
		vec2f worldToScreen(const vec3f& p) const noexcept;
		Ray screenToWorld(const vec2f& screenCoord) noexcept;

		inline const vec2f& getSize() const noexcept { return _size; }
		inline const vec2f& getNearFar() const noexcept { return _near_far; }
		inline const Frustum* getFrustum() const noexcept { return _frustum.get(); }

		inline const vec3f& getScale() const noexcept { return _scale; }
		inline const vec3f& getRotation() const noexcept { return _rotation; }
		inline const vec3f& getPosition() const noexcept { return _position; }

		inline const mat4f& getTransform() const noexcept { return _transform; }
		inline const mat4f& getViewTransform() const noexcept { return _viewTransform; }
		inline const mat4f& getProjectionTransform() const noexcept { return _projectionTransform; }

		inline const mat4f& getInvTransform() noexcept {
			if (_dirty->invTransform) {
				_invTransform = glm::inverse(_transform);
				_dirty->invTransform = 0;
			}
			return _invTransform;
		}

		inline const mat4f& getInvViewTransform() noexcept {
			if (_dirty->invViewTransform) {
				_invViewTransform = glm::inverse(_viewTransform);
				_dirty->invViewTransform = 0;
			}
			return _invViewTransform;
		}

		inline void setRotationOrder(const RotationsOrder ro) noexcept {
			if (_rotationOrder != ro) {
				_rotationOrder = ro;
				_dirty->transform = 1;
			}
		}

		template <typename VEC3 = const vec3f&>
		inline void setScale(VEC3&& s) noexcept {
			if (_scale != s) {
				_scale = s;
				_dirty->transform = 1;
			}
		}

		template <typename VEC3 = const vec3f&>
		inline void setRotation(VEC3&& r) noexcept {
			if (_rotation != r) {
				_rotation = r;
				_dirty->transform = 1;
			}
		}

		template <typename VEC3 = const vec3f&>
		inline void setPosition(VEC3&& p) noexcept {
			if (_position != p) {
				_position = p;
				_dirty->transform = 1;
			}
		}

		template <typename VEC3 = const vec3f&>
		inline void movePosition(VEC3&& p) noexcept {
			if (p != emptyVec3) {
				const vec4f move = vec4f(p, 0.0f) * _transform;

				_position.x += move.x;
				_position.y += move.y;
				_position.z += move.z;

				_dirty->transform = 1;
			}
		}

		inline void enableFrustum() {
			if (_frustum) {
				return;
			}
			_frustum = std::make_unique<Frustum>();
		}

		inline void disableFrustum() {
			if (_frustum) {
				_frustum = nullptr;
			}
		}

		bool calculateTransform() noexcept;

		void makeProjection(const float fov, const float aspect, const float znear, const float zfar) noexcept;
		void makeOrtho(const float left, const float right, const float bottom, const float top, const float znear, const float zfar) noexcept;

		inline void addObserver(ICameraTransformChangeObserver* observer) {
			_observers.push_back(observer);
		}

		inline void removeObserver(ICameraTransformChangeObserver* observer) {
			_observers.erase(std::remove(_observers.begin(), _observers.end(), observer), _observers.end());
		}

		inline void notifyObservers() noexcept {
			for (auto&& observer : _observers) {
				observer->onCameraTransformChanged(this);
			}
		}

	private:
		ProjectionType _projectionType;
		DirtyValue _dirty;

		vec2f _size;
		vec2f _near_far;
		mat4f _transform;
		mat4f _invTransform;
		mat4f _invViewTransform;

		mat4f _viewTransform;
		mat4f _projectionTransform;

		std::unique_ptr<Frustum> _frustum;

		RotationsOrder _rotationOrder;
		vec3f _scale;
		vec3f _rotation;
		vec3f _position;

		std::vector<ICameraTransformChangeObserver*> _observers;
	};
}