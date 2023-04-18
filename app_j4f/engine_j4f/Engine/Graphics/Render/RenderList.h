#pragma once

#include "../Vulkan/vkCommandBuffer.h"
#include "../../Core/Math/mathematic.h"
#include <vector>
#include <cstdint>

namespace engine {

	struct RenderDescriptor;
	struct ViewParams;

	class RenderList {
	public:

		void addDescriptor(RenderDescriptor* d, const uint16_t layer = 0);

		void clear();
		void eraseLayersData();

		void sort();

		void render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const ViewParams& viewParams);
		void render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, ViewParams&& viewParams);
	private:
		std::vector<std::vector<RenderDescriptor*>> _descriptors;
	};
}