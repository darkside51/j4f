#pragma once

#include "../../Core/Math/mathematic.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace engine {

	class Camera;

	struct Ray {
		glm::vec3 s;
		glm::vec3 f;
		Ray(
			const float sx, const float sy, const float sz,
			const float fx, const float fy, const float fz
		) : s(sx, sy, sz), f(fx, fy, fz) {}
	};

	class Frustum {
	public:
		void calculate(const glm::mat4& clip) noexcept;
		
		bool isPointVisible(const glm::vec3& p) const noexcept;
		bool isSphereVisible(const glm::vec3& p, const float r) noexcept;
		bool isCubeVisible_classic(const glm::vec3& min, const glm::vec3& max) const noexcept;
		bool isCubeVisible(const glm::vec3& min, const glm::vec3& max) const noexcept;

	private:
		void normalize() noexcept;
		bool _normalized;
		float _frustum[6][4];
	};

	class FrustumCollection {
	public:
		FrustumCollection(const uint8_t count) : _frustums(count) {}

		inline void calculate(const std::vector<glm::mat4>& clips) noexcept {
			size_t i = 0;
			for (auto&& f : _frustums) {
				f.calculate(clips[i++]);
			}
		}

		void calculateOne(const glm::mat4& clip, const uint8_t idx) noexcept {
			_frustums[idx].calculate(clip);
		}

		inline bool isPointVisible(const glm::vec3& p) const noexcept {
			for (auto&& f : _frustums) { if (f.isPointVisible(p)) return true; }
		}

		inline bool isSphereVisible(const glm::vec3& p, const float r) noexcept {
			for (auto&& f : _frustums) { if (f.isSphereVisible(p, r)) return true; }
		}

		inline bool isCubeVisible_classic(const glm::vec3& min, const glm::vec3& max) const noexcept {
			for (auto&& f : _frustums) { if (f.isCubeVisible_classic(min, max)) return true; }
		}

		inline bool isCubeVisible(const glm::vec3& min, const glm::vec3& max) const noexcept {
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
		Camera(const glm::vec2& sz);

		~Camera() noexcept {
			disableFrustum();
		}

		void resize(const float w, const float h) noexcept;
		glm::vec2 worldToScreen(const glm::vec3& p) const noexcept;
		Ray screenToWorld(const glm::vec2& screenCoord) noexcept;

		inline const glm::vec2& getSize() const noexcept { return _size; }
		inline const glm::vec2& getNearFar() const noexcept { return _near_far; }
		inline const Frustum* getFrustum() const noexcept { return _frustum.get(); }

		inline const glm::vec3& getScale() const noexcept { return _scale; }
		inline const glm::vec3& getRotation() const noexcept { return _rotation; }
		inline const glm::vec3& getPosition() const noexcept { return _position; }

		inline const glm::mat4& getTransform() const noexcept { return _transform; }
		inline const glm::mat4& getViewTransform() const noexcept { return _viewTransform; }
		inline const glm::mat4& getProjectionTransform() const noexcept { return _projectionTransform; }

		inline const glm::mat4& getInvTransform() noexcept {
			if (_dirty->invTransform) {
				_invTransform = glm::inverse(_transform);
				_dirty->invTransform = 0;
			}
			return _invTransform;
		}

		inline const glm::mat4& getInvViewTransform() noexcept {
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

		template <typename VEC3 = const glm::vec3&>
		inline void setScale(VEC3&& s) noexcept {
			if (_scale != s) {
				_scale = s;
				_dirty->transform = 1;
			}
		}

		template <typename VEC3 = const glm::vec3&>
		inline void setRotation(VEC3&& r) noexcept {
			if (_rotation != r) {
				_rotation = r;
				_dirty->transform = 1;
			}
		}

		template <typename VEC3 = const glm::vec3&>
		inline void setPosition(VEC3&& p) noexcept {
			if (_position != p) {
				_position = p;
				_dirty->transform = 1;
			}
		}

		template <typename VEC3 = const glm::vec3&>
		inline void movePosition(VEC3&& p) noexcept {
			if (p != emptyVec3) {
				const glm::vec4 move = glm::vec4(p, 0.0f) * _transform;

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

		glm::vec2 _size;
		glm::vec2 _near_far;
		glm::mat4 _transform;
		glm::mat4 _invTransform;
		glm::mat4 _invViewTransform;

		glm::mat4 _viewTransform;
		glm::mat4 _projectionTransform;

		std::unique_ptr<Frustum> _frustum;

		RotationsOrder _rotationOrder;
		glm::vec3 _scale;
		glm::vec3 _rotation;
		glm::vec3 _position;

		std::vector<ICameraTransformChangeObserver*> _observers;
	};
}