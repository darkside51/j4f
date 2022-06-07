#pragma once

#include "vkCommandBuffer.h"
#include "vkFrameBuffer.h"
#include "vkSynchronisation.h"
#include "vkHelper.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

namespace vulkan {

	struct VulkanRenderTarget {
		uint32_t m_imageCount;
		uint32_t m_width;
		uint32_t m_height;

		std::vector<vulkan::VulkanFrameBuffer> m_frameBuffers;
		VkRenderPass m_renderPass;

		VulkanRenderTarget(
			const uint32_t width, const uint32_t height,
			VulkanDevice* vulkanDevice, const uint32_t imageCount, 
			VkRenderPass renderPass,
			std::vector<VkImageView>& attachments,
			std::vector<VkClearValue>& clearValues
		) : 
			m_width(width), m_height(height),
			m_imageCount(imageCount),
			m_renderPass(renderPass)
		{

			for (size_t i = 0; i < imageCount; ++i) {
				m_frameBuffers.emplace_back(vulkanDevice, m_width, m_height, 1, m_renderPass, attachments.data(), attachments.size());
			}
		}

		~VulkanRenderTarget() { }

	};

}