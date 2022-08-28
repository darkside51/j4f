#pragma once

#include "../../Core/Math/math.h"
#include "Camera.h"

namespace engine {
	struct RenderDescriptor;
	
	enum class BVolumeType : uint8_t {
		CUBE = 0,
		SPHERE = 1,
		CUSTOM = 0xff
	};

	class BVolume {
	public:
		BVolume(const BVolumeType t) : _type(t) {}
		virtual ~BVolume() = default;
		virtual bool checkFrustum(const Frustum* f, const glm::mat4& wtr) const = 0;
		virtual RenderDescriptor* getRenderDescriptor() const { return nullptr; }

		inline BVolumeType type() const { return _type; }

	private:
		BVolumeType _type;
	};

	class CubeVolume : public BVolume {
	public:
		CubeVolume(const glm::vec3& m1, const glm::vec3& m2) : BVolume(BVolumeType::CUBE), _min(m1), _max(m2) { }
		CubeVolume(glm::vec3&& m1, glm::vec3&& m2) : BVolume(BVolumeType::CUBE), _min(std::move(m1)), _max(std::move(m2)) { }

		inline bool checkFrustum(const Frustum* f, const glm::mat4& wtr) const override {
			const glm::vec4 m1 = wtr * glm::vec4(_min.x, _min.y, _min.z, 1.0f);
			const glm::vec4 m2 = wtr * glm::vec4(_max.x, _max.y, _max.z, 1.0f);
			return f->cubeInFrustum(m1, m2);
		}

	private:
		glm::vec3 _min;
		glm::vec3 _max;
	};

	class SphereVolume : public BVolume {
	public:
		SphereVolume(const glm::vec3& c, const float r) : BVolume(BVolumeType::SPHERE), _center(c), _radius(r) {
#ifdef ENABLE_DRAW_BOUNDING_VOLUMES
			createDebugRenderDescriptor();
#endif
		}

		SphereVolume(glm::vec3&& c, const float r) : BVolume(BVolumeType::SPHERE), _center(std::move(c)), _radius(r) {
#ifdef ENABLE_DRAW_BOUNDING_VOLUMES
			createDebugRenderDescriptor();
#endif		
		}

		~SphereVolume() {
#ifdef ENABLE_DRAW_BOUNDING_VOLUMES
			if (_debugRenderDescriptor) {
				delete _debugRenderDescriptor;
				_debugRenderDescriptor = nullptr;
			}
#endif
		}

		inline bool checkFrustum(const Frustum* f, const glm::mat4& wtr) const override {
			const glm::vec4 center = wtr * glm::vec4(_center.x, _center.y, _center.z, 1.0f);
			const float radius = vec_length(wtr[0]) * _radius;
			return (const_cast<Frustum*>(f))->sphereInFrustum(center, radius);
		}

#ifdef ENABLE_DRAW_BOUNDING_VOLUMES
		void createDebugRenderDescriptor();
		RenderDescriptor* getRenderDescriptor() const override { return _debugRenderDescriptor; }
#endif // ENABLE_DRAW_BOUNDING_VOLUMES

	private:
		glm::vec3 _center;
		float _radius;

#ifdef ENABLE_DRAW_BOUNDING_VOLUMES
		RenderDescriptor* _debugRenderDescriptor = nullptr;
#endif // ENABLE_DRAW_BOUNDING_VOLUMES
	};

	class BoundingVolume {
	public:
		~BoundingVolume() {
			if (_impl) {
				delete _impl;
				_impl = nullptr;
			}
		}

		BoundingVolume() = default;
		BoundingVolume(BVolume* impl) : _impl(impl) {}

		template <typename T, typename... Args>
		inline static BoundingVolume* make(Args&&... args) {
			return new BoundingVolume(new T(std::forward<Args>(args)...));;
		}

		inline bool checkFrustum(const Frustum* f, const glm::mat4& wtr) const { 
			// reserved types use checkFrustum call without virtual table
			switch (_impl->type()) {
				case BVolumeType::CUBE:
					return (static_cast<CubeVolume*>(_impl))->checkFrustum(f, wtr);
					break;
				case BVolumeType::SPHERE:
					return (static_cast<SphereVolume*>(_impl))->checkFrustum(f, wtr);
					break;
				default:
					return _impl->checkFrustum(f, wtr); // virtual call
					break;
			}

			return true;
		}

		// debug draw
		inline RenderDescriptor* getRenderDescriptor() const { return _impl->getRenderDescriptor(); /*virtual call*/ }

	private:
		BVolume* _impl = nullptr;
	};

}