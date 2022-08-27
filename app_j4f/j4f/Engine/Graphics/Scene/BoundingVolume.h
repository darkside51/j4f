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
		virtual bool checkFrustum(const Frustum* f) const = 0;
		virtual RenderDescriptor* getRenderDescriptor() const { return nullptr; }

		inline BVolumeType type() const { return _type; }

	private:
		BVolumeType _type;
	};

	class CubeVolume : public BVolume {
	public:
		CubeVolume(const glm::vec3& m1, const glm::vec3& m2) : BVolume(BVolumeType::CUBE), _min(m1), _max(m2) { }
		CubeVolume(glm::vec3&& m1, glm::vec3&& m2) : BVolume(BVolumeType::CUBE), _min(std::move(m1)), _max(std::move(m2)) { }

		inline bool checkFrustum(const Frustum* f) const override {
			return f->cubeInFrustum(_min, _max);
		}

	private:
		glm::vec3 _min;
		glm::vec3 _max;
	};

	class SphereVolume : public BVolume {
	public:
		SphereVolume(const glm::vec3& c, const float r) : BVolume(BVolumeType::SPHERE), _center(c), _radius(r) { }
		SphereVolume(glm::vec3&& c, const float r) : BVolume(BVolumeType::SPHERE), _center(std::move(c)), _radius(r) { }

		inline bool checkFrustum(const Frustum* f) const override {
			return (const_cast<Frustum*>(f))->sphereInFrustum(_center, _radius);
		}

	private:
		glm::vec3 _center;
		float _radius;
	};

	class BoundingVolume {
	public:
		~BoundingVolume() {
			if (_impl) {
				delete _impl;
				_impl = nullptr;
			}
		}

		template <typename T, typename... Args>
		BoundingVolume(Args&&... args) {
			_impl = new T(std::forward<Args>(args)...);
		}

		inline bool checkFrustum(const Frustum* f) const { 
			// reserved types use checkFrustum call without virtual table
			switch (_impl->type()) {
				case BVolumeType::CUBE:
					return (static_cast<CubeVolume*>(_impl))->checkFrustum(f);
					break;
				case BVolumeType::SPHERE:
					return (static_cast<SphereVolume*>(_impl))->checkFrustum(f);
					break;
				default:
					return _impl->checkFrustum(f); // virtual call
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