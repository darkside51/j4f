#include "vkSwapChain.h"
#include "vkDevice.h"
#include <cassert>

#include "../RenderSurfaceInitializer.h"

namespace vulkan {

	void VulkanSwapChain::connect(VkInstance instance, VulkanDevice* vkDevice, const engine::IRenderSurfaceInitializer* initializer) {
		_instance = instance;
		_vkDevice = vkDevice;
		_physicalDevice = vkDevice->gpu;
		_device = vkDevice->device;

		_desiredImagesCount = initializer->getDesiredImageCount();

		initializer->initRenderSurface(&_instance, &surface);

		uint32_t formatCount;
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, surface, &formatCount, nullptr) != VkResult::VK_SUCCESS) {
			// some reaction...
		}

		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, surface, &formatCount, &surfaceFormats[0]);

		// If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
		// there is no preferred format, so we assume VK_FORMAT_B8G8R8A8_UNORM
		if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED)) {
			colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
			colorSpace = surfaceFormats[0].colorSpace;
		} else {
			// iterate over the list of available surface format and
			// check for the presence of VK_FORMAT_B8G8R8A8_UNORM
			//bool has_B8G8R8A8_UNORM = false;
			for (auto&& surfaceFormat : surfaceFormats) {
				if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM) {
					colorFormat = surfaceFormat.format;
					colorSpace = surfaceFormat.colorSpace;
					//has_B8G8R8A8_UNORM = true;
					break;
				} else if (colorFormat == VkFormat::VK_FORMAT_UNDEFINED) {
					colorFormat = surfaceFormat.format;
					colorSpace = surfaceFormat.colorSpace;
				}
			}
		}

        const bool isPresentSupport = _vkDevice->checkPresentSupport(surface);
        assert(isPresentSupport);
	}

	void VulkanSwapChain::resize(const uint32_t width, const uint32_t height, const bool vsync) {
		// store the old swapchain
		VkSwapchainKHR oldSwapchain = swapChain;

		// get physical device surface properties and formats
		VkSurfaceCapabilitiesKHR surfCaps;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, surface, &surfCaps);

		const uint32_t fixedWidth = std::clamp(width, surfCaps.minImageExtent.width, surfCaps.maxImageExtent.width);
		const uint32_t fixedHeight = std::clamp(height, surfCaps.minImageExtent.height, surfCaps.maxImageExtent.height);
		const VkExtent2D swapchainExtent = { fixedWidth , fixedHeight };
        // hmm...
//        const VkExtent2D swapchainExtent = { width, height };

		// select a present mode for the swapchain

		// the VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
		// this mode waits for the vertical blank ("v-sync")
		VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

		// if v-sync is not requested, try to find a mailbox mode
		// it's the lowest latency non-tearing present mode available
		if (!vsync) {
            // get available present modes
            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, surface, &presentModeCount, nullptr);

            std::vector<VkPresentModeKHR> presentModes(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, surface, &presentModeCount, &presentModes[0]);

			for (size_t i = 0; i < presentModeCount; ++i) {
				switch (presentModes[i]) {
					case VK_PRESENT_MODE_MAILBOX_KHR:
						swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
						i = presentModeCount; // for exit from cycle
						break;
					case VK_PRESENT_MODE_IMMEDIATE_KHR:
						swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
						break;
					default:
						break;
				}
			}
		}

		// determine the number of images
		uint32_t desiredSwapchainImagesCount = std::max(_desiredImagesCount, surfCaps.minImageCount);
		if ((surfCaps.maxImageCount > 0) && (desiredSwapchainImagesCount > surfCaps.maxImageCount)) {
			desiredSwapchainImagesCount = surfCaps.maxImageCount;
		}

		// find the transformation of the surface
		VkSurfaceTransformFlagsKHR preTransform;
		if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
			// prefer a non-rotated transform
			preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		} else {
			preTransform = surfCaps.currentTransform;
		}

		// find a supported composite alpha format (not all devices support alpha opaque)
		VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		// select the first composite alpha format available
		const std::array<VkCompositeAlphaFlagBitsKHR, 4> compositeAlphaFlags = {
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
		};

		for (auto& compositeAlphaFlag : compositeAlphaFlags) {
			if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
				compositeAlpha = compositeAlphaFlag;
				break;
			};
		}

		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.pNext = nullptr;
        swapchainCreateInfo.flags = 0;
        swapchainCreateInfo.surface = surface;
        swapchainCreateInfo.minImageCount = desiredSwapchainImagesCount;
        swapchainCreateInfo.imageFormat = colorFormat;
        swapchainCreateInfo.imageColorSpace = colorSpace;
        swapchainCreateInfo.imageExtent = swapchainExtent;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCreateInfo.preTransform = static_cast<VkSurfaceTransformFlagBitsKHR>(preTransform);
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 1;
        swapchainCreateInfo.pQueueFamilyIndices = &_vkDevice->queueFamilyIndices.present;
        swapchainCreateInfo.presentMode = swapchainPresentMode;
		// setting oldSwapChain to the saved handle of the previous swapchain aids in resource reuse and makes sure that we can still present already acquired images
		swapchainCreateInfo.oldSwapchain = oldSwapchain;
		// setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
		swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.compositeAlpha = compositeAlpha;

		// enable transfer source on swap chain images if supported ???????????????????????
		if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
            swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}

		// enable transfer destination on swap chain images if supported ???????????????????????
		if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
            swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		if (vkCreateSwapchainKHR(_device, &swapchainCreateInfo, nullptr, &swapChain) != VkResult::VK_SUCCESS) {
			assert(false);
			// reaction...
		}

		// if an existing swap chain is re-created, destroy the old swap chain
		// this also cleans up all the presentable images
		if (oldSwapchain != VK_NULL_HANDLE) {
			for (uint32_t i = 0; i < imageCount; ++i) {
				vkDestroyImageView(_device, images[i].view, nullptr);
			}

			vkDestroySwapchainKHR(_device, oldSwapchain, nullptr);
		}

		vkGetSwapchainImagesKHR(_device, swapChain, &imageCount, nullptr);

		// get the swap chain images
		std::vector<VkImage> vk_images(imageCount);
		vkGetSwapchainImagesKHR(_device, swapChain, &imageCount, &vk_images[0]);

		images.resize(imageCount);
		for (uint32_t i = 0; i < imageCount; ++i) {
			VkImageViewCreateInfo colorAttachmentView;
			colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			colorAttachmentView.pNext = nullptr;
			colorAttachmentView.format = colorFormat;
			colorAttachmentView.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

			images[i].image = vk_images[i];

			colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			colorAttachmentView.subresourceRange.baseMipLevel = 0;
			colorAttachmentView.subresourceRange.levelCount = 1;
			colorAttachmentView.subresourceRange.baseArrayLayer = 0;
			colorAttachmentView.subresourceRange.layerCount = 1;
			colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
			colorAttachmentView.flags = 0;
			colorAttachmentView.image = images[i].image;

			vkCreateImageView(_device, &colorAttachmentView, nullptr, &images[i].view);
		}
	}
}