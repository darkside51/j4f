#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

namespace engine {
	class IRenderSurfaceInitializer;
}

namespace vulkan {

	class VulkanDevice;

	struct VulkanSwapChainImage {
		VkImage image;
		VkImageView view;
	};

	class VulkanSwapChain {
	public:
		VulkanSwapChain() {
			_presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			_presentInfo.pNext = nullptr;
			_presentInfo.swapchainCount = 1;
			_presentInfo.pSwapchains = &swapChain;
			_presentInfo.pResults = nullptr;
		}

		~VulkanSwapChain() {
			clear();
		}

		VkFormat colorFormat = VkFormat::VK_FORMAT_UNDEFINED;
		uint32_t imageCount = 0;
		std::vector<VulkanSwapChainImage> images;

		void connect(VkInstance instance, VulkanDevice* vkDevice, const engine::IRenderSurfaceInitializer* initializer);

		inline VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex) {
			return vkAcquireNextImageKHR(_device, swapChain, UINT64_MAX,
                                         presentCompleteSemaphore, nullptr, imageIndex);
		}

		inline VkResult queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE) {
			_presentInfo.pImageIndices = &imageIndex;

			if (waitSemaphore != VK_NULL_HANDLE) {
				_presentInfo.pWaitSemaphores = &waitSemaphore;
				_presentInfo.waitSemaphoreCount = 1;
			} else {
				_presentInfo.pWaitSemaphores = nullptr;
				_presentInfo.waitSemaphoreCount = 0;
			}

			return vkQueuePresentKHR(queue, &_presentInfo);
		}

		void resize(const uint32_t width, const uint32_t height, const bool vsync = false);

		inline void clear() {
			if (swapChain != VK_NULL_HANDLE) {
				for (uint32_t i = 0; i < imageCount; ++i) {
					vkDestroyImageView(_device, images[i].view, nullptr);
				}
			}

			if (surface != VK_NULL_HANDLE) {
				vkDestroySwapchainKHR(_device, swapChain, nullptr);
				vkDestroySurfaceKHR(_instance, surface, nullptr);
			}

			surface = VK_NULL_HANDLE;
			swapChain = VK_NULL_HANDLE;
		}

	private:
		VkInstance _instance				= VK_NULL_HANDLE;
		VkPhysicalDevice _physicalDevice	= VK_NULL_HANDLE;
		VkDevice _device					= VK_NULL_HANDLE;
		VulkanDevice* _vkDevice = nullptr;
		VkPresentInfoKHR _presentInfo = {};
		uint32_t _desiredImagesCount = 0;

		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkSwapchainKHR swapChain = VK_NULL_HANDLE;
		VkColorSpaceKHR colorSpace = VkColorSpaceKHR::VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	};
}