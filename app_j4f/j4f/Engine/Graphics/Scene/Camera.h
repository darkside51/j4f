#pragma once

#include "../../Core/Math/math.h"

#include <cstdint>
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
		void calculate(const glm::mat4& clip);
		
		bool pointInFrustum(const glm::vec3& p) const;
		bool sphereInFrustum(const glm::vec3& p, const float r);
		bool cubeInFrustum_classic(const glm::vec3& min, const glm::vec3& max) const;
		bool cubeInFrustum(const glm::vec3& min, const glm::vec3& max) const;

	private:
		void normalize();
		bool _normalized;
		float _frustum[6][4];
	};

	class FrustumCollection {
	public:
		FrustumCollection(const uint8_t count) : _frustums(count) {}

		inline void calculate(const std::vector<glm::mat4>& clips) {
			size_t i = 0;
			for (auto&& f : _frustums) {
				f.calculate(clips[i++]);
			}
		}

		void calculateOne(const glm::mat4& clip, const uint8_t idx) {
			_frustums[idx].calculate(clip);
		}

		inline bool pointInFrustum(const glm::vec3& p) const {
			for (auto&& f : _frustums) { if (f.pointInFrustum(p)) return true; }
		}

		inline bool sphereInFrustum(const glm::vec3& p, const float r) {
			for (auto&& f : _frustums) { if (f.sphereInFrustum(p, r)) return true; }
		}

		inline bool cubeInFrustum_classic(const glm::vec3& min, const glm::vec3& max) const {
			for (auto&& f : _frustums) { if (f.cubeInFrustum_classic(min, max)) return true; }
		}

		inline bool cubeInFrustum(const glm::vec3& min, const glm::vec3& max) const {
			for (auto&& f : _frustums) { if (f.cubeInFrustum(min, max)) return true; }
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

			DirtyFlags* operator->() { return &flags; }
			const DirtyFlags* operator->() const { return &flags; }
		};

	public:
		Camera(const uint32_t w, const uint32_t h);
		Camera(const glm::vec2& sz);

		~Camera() {
			disableFrustum();
		}

		void resize(const float w, const float h);
		glm::vec2 worldToScreen(const glm::vec3& p) const;
		Ray screenToWorld(const glm::vec2& screenCoord);

		inline const glm::vec2& getSize() const { return _size; }
		inline const glm::vec2& getNearFar() const { return _near_far; }
		inline const Frustum* getFrustum() const { return _frustum; }

		inline const glm::vec3& getScale() const { return _scale; }
		inline const glm::vec3& getRotation() const { return _rotation; }
		inline const glm::vec3& getPosition() const { return _position; }
		inline const glm::mat4& getMatrix() const { return _transform; }

		inline const glm::mat4& getViewTransform() const { return _viewTransform; }

		inline const glm::mat4& getInvMatrix() { 
			if (_dirty->invTransform) {
				_invTransform = glm::inverse(_transform);
				_dirty->invTransform = 0;
			}
			return _invTransform;
		}

		inline const glm::mat4& getInvViewMatrix() {
			if (_dirty->invViewTransform) {
				_invViewTransform = glm::inverse(_viewTransform);
				_dirty->invViewTransform = 0;
			}
			return _invViewTransform;
		}

		inline void setRotationOrder(const RotationsOrder ro) {
			if (_rotationOrder != ro) {
				_rotationOrder = ro;
				_dirty->transform = 1;
			}
		}

		template <typename VEC3 = const glm::vec3&>
		inline void setScale(VEC3&& s) {
			if (_scale != s) {
				_scale = s;
				_dirty->transform = 1;
			}
		}

		template <typename VEC3 = const glm::vec3&>
		inline void setRotation(VEC3&& r) {
			if (_rotation != r) {
				_rotation = r;
				_dirty->transform = 1;
			}
		}

		template <typename VEC3 = const glm::vec3&>
		inline void setPosition(VEC3&& p) { 
			if (_position != p) {
				_position = p;
				_dirty->transform = 1;
			}
		}

		template <typename VEC3 = const glm::vec3&>
		inline void movePosition(VEC3&& p) {
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
			_frustum = new Frustum();
		}

		inline void disableFrustum() {
			if (_frustum) {
				delete _frustum;
				_frustum = nullptr;
			}
		}

		bool calculateTransform();

		void makeProjection(const float fov, const float aspect, const float znear, const float zfar);
		void makeOrtho(const float left, const float right, const float bottom, const float top, const float znear, const float zfar);

		inline void addObserver(ICameraTransformChangeObserver* observer) {
			_observers.push_back(observer);
		}

		inline void removeObserver(ICameraTransformChangeObserver* observer) {
			_observers.erase(std::remove(_observers.begin(), _observers.end(), observer), _observers.end());
		}

		inline void notifyObservers() {
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

		Frustum* _frustum;

		RotationsOrder _rotationOrder;
		glm::vec3 _scale;
		glm::vec3 _rotation;
		glm::vec3 _position;

		std::vector<ICameraTransformChangeObserver*> _observers;
	};
}