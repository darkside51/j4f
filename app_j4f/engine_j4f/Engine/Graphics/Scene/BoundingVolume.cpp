#include "BoundingVolume.h"
#include "../Render/RenderDescriptor.h"
#include "../Render/RenderHelper.h"

namespace engine {

	void CubeVolume::render(const mat4f& cameraMatrix, const mat4f& wtr, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame) const {
		Engine::getInstance().getModule<Graphics>().getRenderHelper()->drawBoundingBox(_min, _max, cameraMatrix, wtr, commandBuffer, currentFrame, true);
	}

	void SphereVolume::render(const mat4f& cameraMatrix, const mat4f& wtr, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame) const {
		Engine::getInstance().getModule<Graphics>().getRenderHelper()->drawSphere(_center, _radius, cameraMatrix, wtr, commandBuffer, currentFrame, true);
	}

}