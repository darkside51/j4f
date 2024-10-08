#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace vulkan {
	struct VulkanFence {
		VkFence fence = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;
		const VkAllocationCallbacks* allocator = nullptr;

		VulkanFence() : fence(VK_NULL_HANDLE), device(VK_NULL_HANDLE), allocator(nullptr) { }

		explicit VulkanFence(VkDevice d, const VkFenceCreateFlags flags, const VkAllocationCallbacks* pAllocator = nullptr) : device(d), allocator(pAllocator) {
			VkFenceCreateInfo fenceCreateInfo;
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.pNext = nullptr;
			fenceCreateInfo.flags = flags;
			vkCreateFence(device, &fenceCreateInfo, allocator, &fence);
		}

		VulkanFence(const VulkanFence& f) = default;
		VulkanFence(VulkanFence&& f) noexcept : fence(f.fence), device(f.device), allocator(f.allocator) {
			f.fence = VK_NULL_HANDLE;
		}

		VulkanFence& operator=(VulkanFence&& f) noexcept {
			fence = f.fence;
			device = f.device;
			allocator = f.allocator;
			f.fence = VK_NULL_HANDLE;
			return *this;
		}

		VulkanFence& operator=(const VulkanFence& f) = default;

		~VulkanFence() {
			destroy();
		}

		inline void destroy() {
			if (fence != VK_NULL_HANDLE) {
				vkDestroyFence(device, fence, allocator);
				fence = VK_NULL_HANDLE;
			}
		}
	};

	struct VulkanSemaphore {
		VkSemaphore semaphore = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;
		const VkAllocationCallbacks* allocator;

		VulkanSemaphore() : semaphore(VK_NULL_HANDLE), device(VK_NULL_HANDLE), allocator(nullptr) { }

		explicit VulkanSemaphore(VkDevice d, const VkSemaphoreCreateFlags flags = 0, const VkAllocationCallbacks* pAllocator = nullptr) : device(d), allocator(pAllocator) {
			VkSemaphoreCreateInfo semaphoreCreateInfo;
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			semaphoreCreateInfo.pNext = nullptr;
			semaphoreCreateInfo.flags = flags;
			vkCreateSemaphore(device, &semaphoreCreateInfo, allocator, &semaphore);
		}

		VulkanSemaphore(const VulkanSemaphore& s) = default;
		VulkanSemaphore(VulkanSemaphore&& s) noexcept : semaphore(s.semaphore), device(s.device), allocator(s.allocator) {
			s.semaphore = VK_NULL_HANDLE;
		}

		VulkanSemaphore& operator=(VulkanSemaphore&& s) noexcept {
			semaphore = s.semaphore;
			device = s.device;
			allocator = s.allocator;
			s.semaphore = VK_NULL_HANDLE;
			return *this;
		}

		VulkanSemaphore& operator=(const VulkanSemaphore& s) = default;

		~VulkanSemaphore() {
			if (semaphore != VK_NULL_HANDLE) {
				vkDestroySemaphore(device, semaphore, allocator);
			}
		}

		inline void destroy() {
			if (semaphore != VK_NULL_HANDLE) {
				vkDestroySemaphore(device, semaphore, allocator);
				semaphore = VK_NULL_HANDLE;
			}
		}
	};
}