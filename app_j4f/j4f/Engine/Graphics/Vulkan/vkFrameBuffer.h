#pragma once

#include "vkDevice.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

namespace vulkan {

	/*struct FramebufferAttachment {
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
		VkFormat format;
		VkImageSubresourceRange subresourceRange;
		VkAttachmentDescription description;

		bool hasDepth() const {
			std::vector<VkFormat> formats = {
				VK_FORMAT_D16_UNORM,
				VK_FORMAT_X8_D24_UNORM_PACK32,
				VK_FORMAT_D32_SFLOAT,
				VK_FORMAT_D16_UNORM_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D32_SFLOAT_S8_UINT,
			};

			return std::find(formats.begin(), formats.end(), format) != std::end(formats);
		}

		bool hasStencil() const {
			std::vector<VkFormat> formats = {
				VK_FORMAT_S8_UINT,
				VK_FORMAT_D16_UNORM_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D32_SFLOAT_S8_UINT,
			};

			return std::find(formats.begin(), formats.end(), format) != std::end(formats);
		}

		bool isDepthStencil() const {
			return (hasDepth() || hasStencil());
		}
	};*/

	struct VulkanFrameBuffer {
		VulkanDevice* m_vulkanDevice;
		VkFramebuffer m_framebuffer;
		//std::vector<VkImageView> m_imageViews;
		const VkAllocationCallbacks* m_allocator;

		VulkanFrameBuffer(VulkanDevice* vulkanDevice, const uint32_t w, const uint32_t h,
			const uint32_t layers, VkRenderPass renderPass,
			const VkImageView* imageViews, const uint32_t imageViewsCount, const VkAllocationCallbacks* pAllocator = nullptr) :
			m_vulkanDevice(vulkanDevice), m_allocator(pAllocator) {
			VkFramebufferCreateInfo frameBufferCreateInfo = {};
			frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			// all frame buffers use the same renderpass setup
			frameBufferCreateInfo.renderPass = renderPass;
			frameBufferCreateInfo.attachmentCount = imageViewsCount;
			frameBufferCreateInfo.pAttachments = imageViews;
			frameBufferCreateInfo.width = w;
			frameBufferCreateInfo.height = h;
			frameBufferCreateInfo.layers = layers;
			// create the framebuffer
			vkCreateFramebuffer(m_vulkanDevice->device, &frameBufferCreateInfo, m_allocator, &m_framebuffer);
		}

		/*VulkanFrameBuffer(VulkanDevice* vulkanDevice, const uint32_t w, const uint32_t h,
						const uint32_t layers, VkRenderPass renderPass, 
						std::vector<VkImageView>&& imageViews, const VkAllocationCallbacks* pAllocator = nullptr) : 
						m_vulkanDevice(vulkanDevice), m_imageViews(std::move(imageViews)), m_allocator(pAllocator) {
			VkFramebufferCreateInfo frameBufferCreateInfo = {};
			frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			// all frame buffers use the same renderpass setup
			frameBufferCreateInfo.renderPass = renderPass;
			frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(m_imageViews.size());
			frameBufferCreateInfo.pAttachments = m_imageViews.data();
			frameBufferCreateInfo.width = w;
			frameBufferCreateInfo.height = h;
			frameBufferCreateInfo.layers = layers;
			// create the framebuffer
			vkCreateFramebuffer(m_vulkanDevice->device, &frameBufferCreateInfo, m_allocator, &m_framebuffer);
		}

		VulkanFrameBuffer(VulkanDevice* vulkanDevice, const uint32_t w, const uint32_t h, 
						const uint32_t layers, VkRenderPass renderPass, 
						const std::vector<VkImageView>& imageViews, const VkAllocationCallbacks* pAllocator = nullptr) :
						m_vulkanDevice(vulkanDevice), m_imageViews(imageViews), m_allocator(pAllocator) {
			VkFramebufferCreateInfo frameBufferCreateInfo = {};
			frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			// all frame buffers use the same renderpass setup
			frameBufferCreateInfo.renderPass = renderPass;
			frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(m_imageViews.size());
			frameBufferCreateInfo.pAttachments = m_imageViews.data();
			frameBufferCreateInfo.width = w;
			frameBufferCreateInfo.height = h;
			frameBufferCreateInfo.layers = layers;
			// create the framebuffer
			vkCreateFramebuffer(m_vulkanDevice->device, &frameBufferCreateInfo, m_allocator, &m_framebuffer);
		}*/

		~VulkanFrameBuffer() {
			vkDestroyFramebuffer(m_vulkanDevice->device, m_framebuffer, m_allocator);
		};
	};

}