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
		uint8_t count;

		VulkanDescriptorSet(const uint32_t size = 0) : set(static_cast<size_t>(size)), count(size) { }
		~VulkanDescriptorSet() = default;

		VkDescriptorSet& operator[] (const uint32_t i) { return set[i % count]; }
		const VkDescriptorSet& operator[] (const uint32_t i) const { return set[i % count]; }

		VkDescriptorSet& at(const uint32_t i) { return set[i % count]; }
		const VkDescriptorSet& at(const uint32_t i) const { return set[i % count]; }

		VkDescriptorSet* operator() (const uint32_t i) { return &set[i % count]; }
		const VkDescriptorSet* operator() (const uint32_t i) const { return &set[i % count]; }

		VkDescriptorSet* p_at(const uint32_t i) { return &set[i % count]; }
		const VkDescriptorSet* p_at(const uint32_t i) const { return &set[i % count]; }
	};
}
