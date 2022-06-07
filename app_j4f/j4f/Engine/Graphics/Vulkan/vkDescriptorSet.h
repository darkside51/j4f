#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

namespace vulkan {

	struct VulkanPushConstant {
		VkPushConstantRange* range;
		uint8_t values[128];
	};

	struct VulkanDescriptorSet {
		VkDescriptorPool parentPool;
		std::vector<VkDescriptorSet> set; // размер по количеству буфферов для кадра

		VulkanDescriptorSet(const uint32_t size = 0) : set(static_cast<size_t>(size)) { }
		~VulkanDescriptorSet() = default;

		VkDescriptorSet& operator[] (const uint32_t i) { return set[i]; }
		const VkDescriptorSet& operator[] (const uint32_t i) const { return set[i]; }

		VkDescriptorSet& at(const uint32_t i) { return set[i]; }
		const VkDescriptorSet& at(const uint32_t i) const { return set[i]; }

		VkDescriptorSet* operator() (const uint32_t i) { return &set[i]; }
		const VkDescriptorSet* operator() (const uint32_t i) const { return &set[i]; }

		VkDescriptorSet* p_at(const uint32_t i) { return &set[i]; }
		const VkDescriptorSet* p_at(const uint32_t i) const { return &set[i]; }
	};
}
