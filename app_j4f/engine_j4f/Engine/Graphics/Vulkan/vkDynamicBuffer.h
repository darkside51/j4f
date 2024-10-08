#pragma once

#include "vkBuffer.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <atomic>
#include <cassert>

namespace vulkan {

	struct VulkanDynamicBuffer {
		uint32_t alignedSize;
		uint32_t limit;
		std::atomic_uint32_t offset;
		std::vector<vulkan::VulkanBuffer> buffers;
		std::vector<void*> memory;
        uint8_t mapped = 0u;
		VulkanDynamicBuffer(const uint32_t size, const uint32_t count, const uint32_t lim) : alignedSize(size), buffers(count), memory(count), limit(lim) {
			offset.store(0, std::memory_order_relaxed);
		}

		[[nodiscard]] inline uint32_t getCurrentOffset() const noexcept {
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
            if (mapped == 0xffu) return;
			for (size_t i = 0, sz = buffers.size(); i < sz; ++i) {
                if (!(mapped & (1u << i))) {
                    memory[i] = buffers[i].map(buffers[i].m_size);
                }
			}
            mapped = 0xffu;
		}

		inline void unmap() {
            if (!mapped) return;
            uint8_t i = 0u;
			for (auto&& buffer : buffers) {
                if (mapped & (1u << i)) {
                    buffer.unmap();
                }
                ++i;
			}
            mapped = 0u;
		}

        inline void map(const uint8_t i) {
            if (mapped & (1u << i)) return;
            mapped |= (1u << i);
            memory[i] = buffers[i].map(buffers[i].m_size);
        }

        inline void unmap(const uint8_t i) {
            if (!(mapped & (1u << i))) return;
            mapped &= ~(1u << i);
            buffers[i].unmap();
        }
	};
}