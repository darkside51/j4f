#pragma once

#include "../../Vulkan/vkCommandBuffer.h"

namespace engine {

	class CascadeShadowMap;
	class RenderList;

	void renderShadowMap(CascadeShadowMap* shadowMap, RenderList& list, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame);
	void renderShadowMap(CascadeShadowMap* shadowMap, RenderList** list, const uint32_t count, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame);
}