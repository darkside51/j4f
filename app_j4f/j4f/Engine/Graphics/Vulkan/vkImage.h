#pragma once

#include <vulkan/vulkan.h>
#include <utility>

namespace vulkan {

	class VulkanDevice;

	struct VulkanImage {
		VkImage image = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VulkanDevice* vulkanDevice = nullptr;

		VkImageUsageFlags usage;
		VkImageType imageType;
		VkFormat format;
		uint32_t mipLevels;
		uint32_t arrayLayers;

		VulkanImage() = default;

		VulkanImage(
			VulkanDevice* vulkan_device,
			VkImageUsageFlags image_usage,
			VkImageType image_type,
			VkFormat fmt, uint32_t mip_levels, uint32_t width, uint32_t height, uint32_t depth = 1,
			VkImageCreateFlags flags = 0, uint32_t array_layers = 1,
			VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL
		);

		~VulkanImage();

		VulkanImage(VulkanImage&& img) noexcept : vulkanDevice(img.vulkanDevice), image(std::move(img.image)), view(std::move(img.view)), memory(std::move(img.memory)),
			usage(img.usage), imageType(img.imageType), format(img.format), mipLevels(img.mipLevels), arrayLayers(img.arrayLayers) {
			img.vulkanDevice = nullptr;
			img.view = VK_NULL_HANDLE;
			img.image = VK_NULL_HANDLE;
			img.memory = VK_NULL_HANDLE;
		}

		VulkanImage& operator= (VulkanImage&& img) noexcept {
			vulkanDevice = img.vulkanDevice;
			image = std::move(img.image);
			view = std::move(img.view);
			memory = std::move(img.memory);

			usage = img.usage;
			imageType = img.imageType;
			format = img.format;
			mipLevels = img.mipLevels;
			arrayLayers = img.arrayLayers;

			img.vulkanDevice = nullptr;
			img.view = VK_NULL_HANDLE;
			img.image = VK_NULL_HANDLE;
			img.memory = VK_NULL_HANDLE;

			return *this;
		}

		VulkanImage(const VulkanImage& img) = delete;
		VulkanImage& operator= (const VulkanImage& img) = delete;

		void destroy();
		void createImageView(const VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM, const VkImageAspectFlagBits aspectFlags = VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM);
	};
}