#pragma once

#include "../../Core/Math/math.h"
#include "../Vulkan/vkRenderData.h"
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
		virtual void render(const glm::mat4& cameraMatrix, const glm::mat4& wtr, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame) const { }

		inline BVolumeType type() const { return _type; }

	private:
		BVolumeType _type;
	};

	class CubeVolume : public BVolume {
	public:
		CubeVolume(const glm::vec3& m1, const glm::vec3& m2) : BVolume(BVolumeType::CUBE), _min(m1), _max(m2) { }
		CubeVolume(glm::vec3&& m1, glm::vec3&& m2) : BVolume(BVolumeType::CUBE), _min(std::move(m1)), _max(std::move(m2)) { }

		inline bool checkFrustum(const Frustum* f, const glm::mat4& wtr) const override {
			//const glm::vec4 m1 = wtr * glm::vec4(_min.x, _min.y, _min.z, 1.0f);
			//const glm::vec4 m2 = wtr * glm::vec4(_max.x, _max.y, _max.z, 1.0f);

			glm::vec4 m1(FLT_MAX);
			glm::vec4 m2(-FLT_MAX);
			glm::vec4 m[8];

			m[0] = wtr * glm::vec4(_min.x, _min.y, _min.z, 1.0f);
			m[1] = wtr * glm::vec4(_min.x, _min.y, _max.z, 1.0f);
			m[2] = wtr * glm::vec4(_min.x, _max.y, _min.z, 1.0f);
			m[3] = wtr * glm::vec4(_max.x, _min.y, _min.z, 1.0f);

			m[4] = wtr * glm::vec4(_min.x, _max.y, _max.z, 1.0f);
			m[5] = wtr * glm::vec4(_max.x, _min.y, _max.z, 1.0f);
			m[6] = wtr * glm::vec4(_max.x, _max.y, _min.z, 1.0f);
			m[7] = wtr * glm::vec4(_max.x, _max.y, _max.z, 1.0f);

			for (uint8_t i = 0; i < 8; ++i) {
				m1.x = std::min(m1.x, m[i].x); m1.y = std::min(m1.y, m[i].y); m1.z = std::min(m1.z, m[i].z);
				m2.x = std::max(m2.x, m[i].x); m2.y = std::max(m2.y, m[i].y); m2.z = std::max(m2.z, m[i].z);
			}

			return f->cubeInFrustum(m1, m2);
		}

		void render(const glm::mat4& cameraMatrix, const glm::mat4& wtr, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame) const override;

	private:
		glm::vec3 _min;
		glm::vec3 _max;
	};

	class SphereVolume : public BVolume {
	public:
		SphereVolume(const glm::vec3& c, const float r) : BVolume(BVolumeType::SPHERE), _center(c), _radius(r) {}
		SphereVolume(glm::vec3&& c, const float r) : BVolume(BVolumeType::SPHERE), _center(std::move(c)), _radius(r) {}

		inline bool checkFrustum(const Frustum* f, const glm::mat4& wtr) const override {
			const glm::vec4 center = wtr * glm::vec4(_center.x, _center.y, _center.z, 1.0f);
			const float radius = vec_length(wtr[0]) * _radius;
			return (const_cast<Frustum*>(f))->sphereInFrustum(center, radius);
		}

		void render(const glm::mat4& cameraMatrix, const glm::mat4& wtr, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame) const override;

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
		inline void render(const glm::mat4& cameraMatrix, const glm::mat4& wtr, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame) const {
			_impl->render(cameraMatrix, wtr, commandBuffer, currentFrame);
		}

	private:
		BVolume* _impl = nullptr;
	};

}