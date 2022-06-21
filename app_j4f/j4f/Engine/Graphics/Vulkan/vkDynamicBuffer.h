#pragma once

#include "vkBuffer.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <atomic>
#include <assert.h>

namespace vulkan {

	struct VulkanDynamicBuffer {
		uint32_t alignedSize;
		uint32_t limit;
		std::atomic_uint32_t offset;
		std::vector<vulkan::VulkanBuffer> buffers;
		std::vector<void*> memory;
		VulkanDynamicBuffer(const uint32_t size, const uint32_t count, const uint32_t lim) : alignedSize(size), buffers(count), memory(count), limit(lim) {
			offset.store(0, std::memory_order_relaxed);
		}

		inline uint32_t getCurrentOffset() const {
			return offset.load(std::memory_order_consume);
		}

		inline uint32_t encrease() {
			const uint32_t last = offset.fetch_add(1, std::memory_order_release);
			if (last + 1 == limit) {
				assert(false); // max size; todo: reaction
			}
			return last;
		}

		inline void resetOffset() {
			offset.store(0, std::memory_order_release);
		}

		inline void map() {
			for (size_t i = 0, sz = buffers.size(); i < sz; ++i) {
				memory[i] = buffers[i].map(buffers[i].m_size);
			}
		}

		inline void unmap() const {
			for (size_t i = 0, sz = buffers.size(); i < sz; ++i) {
				buffers[i].unmap();
			}
		}
	};
}