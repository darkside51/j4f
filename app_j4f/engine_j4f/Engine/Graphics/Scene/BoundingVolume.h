#pragma once

#include "../../Core/Math/mathematic.h"
#include "../Vulkan/vkRenderData.h"
#include "../Camera.h"

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
		virtual bool checkVisible(const void* visibleChecker, const mat4f& wtr) const = 0;
		virtual void render(const mat4f& cameraMatrix, const mat4f& wtr, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame) const { }

		inline BVolumeType type() const { return _type; }

	private:
		BVolumeType _type;
	};

	class CubeVolume : public BVolume {
	public:
		CubeVolume(const vec3f& m1, const vec3f& m2) : BVolume(BVolumeType::CUBE), _min(m1), _max(m2) { }
		CubeVolume(vec3f&& m1, vec3f&& m2) : BVolume(BVolumeType::CUBE), _min(std::move(m1)), _max(std::move(m2)) { }

		template <typename T>
		bool checkVisible(const T* visibleChecker, const mat4f& wtr) const {
			vec4f m1(FLT_MAX);
			vec4f m2(-FLT_MAX);
			vec4f m[8];

			m[0] = wtr * vec4f(_min.x, _min.y, _min.z, 1.0f);
			m[1] = wtr * vec4f(_min.x, _min.y, _max.z, 1.0f);
			m[2] = wtr * vec4f(_min.x, _max.y, _min.z, 1.0f);
			m[3] = wtr * vec4f(_max.x, _min.y, _min.z, 1.0f);

			m[4] = wtr * vec4f(_min.x, _max.y, _max.z, 1.0f);
			m[5] = wtr * vec4f(_max.x, _min.y, _max.z, 1.0f);
			m[6] = wtr * vec4f(_max.x, _max.y, _min.z, 1.0f);
			m[7] = wtr * vec4f(_max.x, _max.y, _max.z, 1.0f);

			for (uint8_t i = 0; i < 8; ++i) {
				m1.x = std::min(m1.x, m[i].x); m1.y = std::min(m1.y, m[i].y); m1.z = std::min(m1.z, m[i].z);
				m2.x = std::max(m2.x, m[i].x); m2.y = std::max(m2.y, m[i].y); m2.z = std::max(m2.z, m[i].z);
			}

			return visibleChecker->isCubeVisible(m1, m2);
		}

		inline bool checkVisible(const void* visibleChecker, const mat4f& wtr) const override { return true; }

		void render(const mat4f& cameraMatrix, const mat4f& wtr, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame) const override;

	private:
		vec3f _min;
		vec3f _max;
	};

	class SphereVolume : public BVolume {
	public:
		SphereVolume(const vec3f& c, const float r) : BVolume(BVolumeType::SPHERE), _center(c), _radius(r) {}
		SphereVolume(vec3f&& c, const float r) : BVolume(BVolumeType::SPHERE), _center(std::move(c)), _radius(r) {}

		template <typename T>
		inline bool checkVisible(const T* visibleChecker, const mat4f& wtr) const {
			const vec4f center = wtr * vec4f(_center.x, _center.y, _center.z, 1.0f);
			const float radius = vec_length(wtr[0]) * _radius;
			return (const_cast<T*>(visibleChecker))->isSphereVisible(center, radius);
		}

		inline bool checkVisible(const void* visibleChecker, const mat4f& wtr) const override { return true; }

		void render(const mat4f& cameraMatrix, const mat4f& wtr, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame) const override;

	private:
		vec3f _center;
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

		BoundingVolume() = default;
		BoundingVolume(BVolume* impl) : _impl(impl) {}

		template <typename T, typename... Args>
		inline static BoundingVolume* make(Args&&... args) {
			return new BoundingVolume(new T(std::forward<Args>(args)...));;
		}

		template <typename T>
		inline bool checkVisible(const T* visibleChecker, const mat4f& wtr) const {
			// reserved types use checkVisible call without virtual table
			switch (_impl->type()) {
				case BVolumeType::CUBE:
					return (static_cast<CubeVolume*>(_impl))->checkVisible<T>(visibleChecker, wtr);
					break;
				case BVolumeType::SPHERE:
					return (static_cast<SphereVolume*>(_impl))->checkVisible<T>(visibleChecker, wtr);
					break;
				default:
					return _impl->checkVisible(visibleChecker, wtr); // virtual call
					break;
			}

			return true;
		}

		// debug draw
		inline void render(const mat4f& cameraMatrix, const mat4f& wtr, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame) const {
			_impl->render(cameraMatrix, wtr, commandBuffer, currentFrame);
		}

	private:
		BVolume* _impl = nullptr;
	};

}