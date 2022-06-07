#include "vkImage.h"
#include "vkDevice.h"
#include "vkHelper.h"

namespace vulkan {

	VulkanImage::VulkanImage(
		VulkanDevice* vulkan_device,
		VkImageUsageFlags image_usage,
		VkImageType image_type,
		VkFormat fmt, uint32_t mip_levels, uint32_t width, uint32_t height, uint32_t depth,
		VkImageCreateFlags flags, uint32_t array_layers,
		VkSampleCountFlagBits samples, VkImageTiling tiling
	) : vulkanDevice(vulkan_device), usage(image_usage), imageType(image_type), format(fmt), mipLevels(mip_levels), arrayLayers(array_layers)
	{
		VkImageCreateInfo imageCI{};
		imageCI.pNext = nullptr;
		imageCI.flags = flags;
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.imageType = imageType;
		imageCI.format = format;
		imageCI.extent = { width, height, depth };
		imageCI.mipLevels = mipLevels;
		imageCI.arrayLayers = arrayLayers;
		imageCI.samples = samples;
		imageCI.tiling = tiling;
		imageCI.usage = usage;
		imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		vkCreateImage(vulkanDevice->device, &imageCI, nullptr, &image);
		VkMemoryRequirements memReqs{};
		vkGetImageMemoryRequirements(vulkanDevice->device, image, &memReqs);

		VkMemoryAllocateInfo memAlloc{};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		vkAllocateMemory(vulkanDevice->device, &memAlloc, nullptr, &memory);
		vkBindImageMemory(vulkanDevice->device, image, memory, 0);
	}

	void VulkanImage::createImageView(const VkImageViewType viewType, const VkImageAspectFlagBits aspectFlags) {
		VkImageViewCreateInfo imageViewCI{};
		imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCI.viewType = (viewType == VK_IMAGE_VIEW_TYPE_MAX_ENUM ? static_cast<VkImageViewType>(imageType) : viewType);
		imageViewCI.image = image;
		imageViewCI.format = format;
		imageViewCI.subresourceRange.baseMipLevel = 0;
		imageViewCI.subresourceRange.levelCount = mipLevels;
		imageViewCI.subresourceRange.baseArrayLayer = 0;
		imageViewCI.subresourceRange.layerCount = arrayLayers;

		if (aspectFlags == VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM) {
			if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
				imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}

			if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
				if (vulkan::hasDepth(format)) {
					imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				}

				if (vulkan::hasStencil(format)) {
					imageViewCI.subresourceRange.aspectMask = imageViewCI.subresourceRange.aspectMask | VK_IMAGE_ASPECT_STENCIL_BIT;
				}
			}
		} else {
			imageViewCI.subresourceRange.aspectMask = aspectFlags;
		}

		vkCreateImageView(vulkanDevice->device, &imageViewCI, nullptr, &view);
	}

	VulkanImage::~VulkanImage() {
		if (vulkanDevice) {
			if (view != VK_NULL_HANDLE) {
				vkDestroyImageView(vulkanDevice->device, view, nullptr);
			}

			vkDestroyImage(vulkanDevice->device, image, nullptr);
			vkFreeMemory(vulkanDevice->device, memory, nullptr);
		}
	}

	void VulkanImage::destroy() {
		if (vulkanDevice) {
			if (view != VK_NULL_HANDLE) {
				vkDestroyImageView(vulkanDevice->device, view, nullptr);
			}

			vkDestroyImage(vulkanDevice->device, image, nullptr);
			vkFreeMemory(vulkanDevice->device, memory, nullptr);
			view = VK_NULL_HANDLE;
			vulkanDevice = nullptr;
		}
	}

}