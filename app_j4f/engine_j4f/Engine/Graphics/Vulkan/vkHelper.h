#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vector>
#include <cstdint>
#include <assert.h>
#include <algorithm>

namespace vulkan {

	class VulkanDevice;

	inline void getVulkanInstanceSupportedLayers(std::vector<VkLayerProperties>& layers) {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		layers.resize(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
	}

	inline void getVulkanInstanceSupportedExtensions(std::vector<VkExtensionProperties>& extensions) {
		uint32_t extensionCount = 0U;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		extensions.resize(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
	}

	inline void getVulkanGPUExtensionProperties(VkPhysicalDevice gpu, std::vector<VkExtensionProperties>& deviceExtProperties) {
		uint32_t deviceExtCount = 0;
		vkEnumerateDeviceExtensionProperties(gpu, nullptr, &deviceExtCount, nullptr);

		deviceExtProperties.resize(deviceExtCount);
		vkEnumerateDeviceExtensionProperties(gpu, nullptr, &deviceExtCount, deviceExtProperties.data());
	}

	inline void getVulkanSurfaceFormats(VkPhysicalDevice gpu, VkSurfaceKHR surface, std::vector<VkSurfaceFormatKHR>& surfaceFormats) {
		uint32_t surfaceFormatsCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &surfaceFormatsCount, nullptr);

		surfaceFormats.resize(surfaceFormatsCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &surfaceFormatsCount, surfaceFormats.data());
	}


	inline bool hasDepth(VkFormat format) {
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

	inline bool hasStencil(VkFormat format) {
		std::vector<VkFormat> formats = {
			VK_FORMAT_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
		};

		return std::find(formats.begin(), formats.end(), format) != std::end(formats);
	}

	inline bool isDepthStencil(VkFormat format) {
		return (hasDepth(format) || hasStencil(format));
	}

	inline VkRenderPassBeginInfo createRenderPassBeginInfo(VkRenderPass renderPass, VkRect2D renderArea, const VkClearValue* clearValues, const uint8_t clearValuesCount, VkFramebuffer frameBuffer = VK_NULL_HANDLE) {
		VkRenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;

		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.framebuffer = frameBuffer;
		renderPassBeginInfo.renderArea = renderArea;
		renderPassBeginInfo.clearValueCount = clearValuesCount;
		renderPassBeginInfo.pClearValues = clearValues;
		return renderPassBeginInfo;
	}

	inline VkFenceCreateInfo createFenceCreateInfo(const VkFenceCreateFlags signaled = 0) {
		VkFenceCreateInfo fenceInfo;
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.pNext = nullptr;
		fenceInfo.flags = signaled;
		return fenceInfo;
	}

	inline void waitFenceAndDestroy(VkDevice device, const VkFence& fence, VkBool32 waitAll = VK_TRUE, const VkAllocationCallbacks* pAllocator = nullptr) {
		vkWaitForFences(device, 1, &fence, waitAll, UINT64_MAX);
		vkDestroyFence(device, fence, pAllocator);
	}

	inline VkResult waitFenceAndReset(VkDevice device, const VkFence& fence, VkBool32 waitAll = VK_TRUE) {
		const VkResult waitResult = vkWaitForFences(device, 1, &fence, waitAll, UINT64_MAX);

        switch (waitResult) {
            case VK_SUCCESS:
                break;
            default:
                return waitResult;
        }

		return vkResetFences(device, 1, &fence);
	}

	inline VkWriteDescriptorSet initWriteDescriptorSet(VkDescriptorType type, const uint32_t count, const uint32_t binding, VkDescriptorSet targetSet, const void* descriptor, const uint32_t targetElement = 0) {
		VkWriteDescriptorSet set;
		set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		set.pNext = nullptr;
		set.descriptorType = type;
		set.descriptorCount = count;
		set.dstBinding = binding;
		set.dstSet = targetSet;
		set.dstArrayElement = targetElement;
		set.pBufferInfo = nullptr;
		set.pImageInfo = nullptr;
		set.pTexelBufferView = nullptr;

		switch (type) {
		case VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		case VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		case VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			set.pBufferInfo = static_cast<const VkDescriptorBufferInfo*>(descriptor);
			break;
		case VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			set.pImageInfo = static_cast<const VkDescriptorImageInfo*>(descriptor);
			break;
		case VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: // ?
		case VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			set.pTexelBufferView = static_cast<const VkBufferView*>(descriptor);
			break;
		default:
			break;
		}

		return set;
	}

	inline VkAttachmentDescription createAttachmentDescription(VkFormat format,
															VkImageLayout initialLayout,
															VkImageLayout finalLayout,
															VkAttachmentLoadOp loadOp,
															VkAttachmentStoreOp storeOp,
															VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
															VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
															VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
															VkAttachmentDescriptionFlags flags = 0
															) {
		VkAttachmentDescription description;

		description.flags = flags;
		description.format = format;
		description.samples = samples;
		description.loadOp = loadOp;
		description.storeOp = storeOp;
		description.stencilLoadOp = stencilLoadOp;
		description.stencilStoreOp = stencilStoreOp;
		description.initialLayout = initialLayout;
		description.finalLayout = finalLayout;

		return description;
	}

	inline VkSubpassDescription createSubpassDescription(
		VkPipelineBindPoint pipelineBindPoint, 
		const std::vector<VkAttachmentReference>& colorAttachments, 
		const VkAttachmentReference* depthStencilAttachment,
		const VkSubpassDescriptionFlags flags = 0,
		const uint32_t inputAttachmentCount = 0,
		const VkAttachmentReference* pInputAttachments = nullptr,
		const VkAttachmentReference* pResolveAttachments = nullptr,
		const uint32_t preserveAttachmentCount = 0,
		const uint32_t* pPreserveAttachments = nullptr
	) {
		VkSubpassDescription description;

		description.flags = flags;
		description.pipelineBindPoint = pipelineBindPoint;

		description.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
		description.pColorAttachments = colorAttachments.data();
		description.pDepthStencilAttachment = depthStencilAttachment;

		description.inputAttachmentCount = inputAttachmentCount;
		description.pInputAttachments = pInputAttachments;

		description.pResolveAttachments = pResolveAttachments;

		description.preserveAttachmentCount = preserveAttachmentCount;
		description.pPreserveAttachments = pPreserveAttachments;

		return description;
	}

	VkRenderPass createRenderPass(
		VulkanDevice* device,
		VkPipelineBindPoint pipelineBindPoint,
		const std::vector<VkSubpassDependency>& dependencies,
		const std::vector<VkAttachmentDescription>& attachments,
		const VkSubpassDescriptionFlags flags = 0,
		const uint32_t inputAttachmentCount = 0,
		const VkAttachmentReference* pInputAttachments = nullptr,
		const VkAttachmentReference* pResolveAttachments = nullptr,
		const uint32_t preserveAttachmentCount = 0,
		const uint32_t* pPreserveAttachments = nullptr);
}