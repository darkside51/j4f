#pragma once

#include "../../Core/Common.h"
#include <vulkan/vulkan.h>
#include <cstring>

namespace vulkan {

	struct VulkanBuffer {
		VkDevice m_device = VK_NULL_HANDLE;
		VkBuffer m_buffer = VK_NULL_HANDLE;
		VkDeviceMemory m_memory = VK_NULL_HANDLE;
		VkDeviceSize m_size = 0;
		VkDeviceSize m_alignment = 0;

		VkBufferUsageFlags m_usage = 0;
		VkMemoryPropertyFlags m_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		~VulkanBuffer() {
			destroy();
		}

		void upload(const void* data, const size_t size, const size_t offset, const bool flushBuffer = false) const {
			void* memory = nullptr;
			vkMapMemory(m_device, m_memory, static_cast<VkDeviceSize>(offset), static_cast<VkDeviceSize>(size), 0, &memory);
			memcpy(memory, data, size);
			if (flushBuffer) {
                const auto result = flush(size, offset);
            } // need when host write to non coherent memory
			vkUnmapMemory(m_device, m_memory);
		}
		
		[[nodiscard]] void* map(const size_t size, const size_t offset = 0) const {
			void* memory = nullptr;
			vkMapMemory(m_device, m_memory, static_cast<VkDeviceSize>(offset), static_cast<VkDeviceSize>(size), 0, &memory);
			return memory;
		}

		void unmap() const { vkUnmapMemory(m_device, m_memory); }

        [[nodiscard]] inline VkResult flush(const VkDeviceSize size = VK_WHOLE_SIZE, const VkDeviceSize offset = 0) const {
			VkMappedMemoryRange mappedRange;
			mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mappedRange.pNext = nullptr;
			mappedRange.memory = m_memory;
			mappedRange.offset = offset;
			mappedRange.size = size;
			return vkFlushMappedMemoryRanges(m_device, 1, &mappedRange);
		}

        [[nodiscard]] inline VkResult invalidate(const VkDeviceSize size = VK_WHOLE_SIZE, const VkDeviceSize offset = 0) const {
			VkMappedMemoryRange mappedRange = {};
			mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mappedRange.memory = m_memory;
			mappedRange.offset = offset;
			mappedRange.size = size;
			return vkInvalidateMappedMemoryRanges(m_device, 1, &mappedRange);
		}

		inline void destroy() {
			if (m_buffer) {
				vkDestroyBuffer(m_device, m_buffer, nullptr);
				m_buffer = VK_NULL_HANDLE;
			}

			if (m_memory) {
				vkFreeMemory(m_device, m_memory, nullptr);
				m_memory = VK_NULL_HANDLE;
			}
		}

        [[nodiscard]] inline bool isValid() const noexcept {
			return (m_buffer != VK_NULL_HANDLE) && (m_memory != VK_NULL_HANDLE);
		}
	};

	class VulkanDevice;

	struct VulkanDeviceBuffer {
		vulkan::VulkanBuffer m_stageBuffer;
		vulkan::VulkanBuffer m_gpuBuffer;

		void upload(void* data, const size_t size, const size_t offset, const bool flushBuffer = false) const {
			if (!m_stageBuffer.isValid()) return;
			void* memory = nullptr;
			vkMapMemory(m_stageBuffer.m_device, m_stageBuffer.m_memory, static_cast<VkDeviceSize>(offset), static_cast<VkDeviceSize>(size), 0, &memory);
			memcpy(memory, data, size);
			if (flushBuffer) {
                const auto result = m_stageBuffer.flush(size, offset);
            } // need when host write to non coherent memory
			vkUnmapMemory(m_stageBuffer.m_device, m_stageBuffer.m_memory);
		}

        [[nodiscard]] void* map(const size_t size, const size_t offset = 0) const {
			if (!m_stageBuffer.isValid()) return nullptr;

			void* memory = nullptr;
			vkMapMemory(m_stageBuffer.m_device, m_stageBuffer.m_memory, static_cast<VkDeviceSize>(offset), static_cast<VkDeviceSize>(size), 0, &memory);
			return memory;
		}

		void unmap() const { 
			if (!m_stageBuffer.isValid()) return;
			vkUnmapMemory(m_stageBuffer.m_device, m_stageBuffer.m_memory);
		}

		void createStageBuffer(VulkanDevice* device, const VkSharingMode sharingMode, const VkDeviceSize size);

		inline void destroyStageBuffer() {
			m_stageBuffer.destroy();
		}

		inline void destroy() {
			m_stageBuffer.destroy();
			m_gpuBuffer.destroy();
		}
	};
}