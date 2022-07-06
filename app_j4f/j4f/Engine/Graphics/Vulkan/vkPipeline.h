#pragma once

#include "vkGPUProgram.h"
#include <vulkan/vulkan.h>
#include <cstdint>

namespace vulkan {

	struct VulkanPipeline {
		VkPipeline pipeline;
		const VulkanGpuProgram* program = nullptr;
		VkRenderPass renderPass;
		uint32_t subpass;

		VulkanPipeline() : pipeline(VK_NULL_HANDLE), renderPass(VK_NULL_HANDLE), subpass(0) { }

		VulkanPipeline(VulkanPipeline&& p) noexcept : pipeline(p.pipeline), program(p.program), renderPass(p.renderPass), subpass(p.subpass) {
			p.pipeline = VK_NULL_HANDLE;
			p.program = nullptr;
			p.renderPass = VK_NULL_HANDLE;
		}

		VulkanPipeline& operator= (VulkanPipeline&& p) noexcept {
			pipeline = p.pipeline;
			program = p.program;
			renderPass = p.renderPass;
			subpass = p.subpass;

			p.pipeline = VK_NULL_HANDLE;
			p.program = nullptr;
			p.renderPass = VK_NULL_HANDLE;

			return *this;
		}

		VulkanPipeline(const VulkanPipeline& p) = delete;
		VulkanPipeline& operator= (const VulkanPipeline& p) = delete;

		void fillDescriptorSets(const uint32_t frameNum, VkDescriptorSet* descriptorSets, const VkDescriptorSet* additionalSets, const uint8_t setsCount) const {
			const uint8_t gpuSets = program->getGPUSetsNumbers();
			uint8_t j = 0;
			for (uint8_t i = 0; i < setsCount; ++i) {
				if (gpuSets & (1 << i)) {
					const VulkanDescriptorSet* set = program->getDescriptorSet(i);
					descriptorSets[i] = set->operator[](frameNum);
				} else if (additionalSets) {
					descriptorSets[i] = additionalSets[j++];
				}
			}
		}

		const VkDescriptorSet& getDescriptorSet(const uint32_t frameNum, const uint32_t setNum = 0) const {
			if (const VulkanDescriptorSet* set = program->getDescriptorSet(setNum)) {
				return set->operator[](frameNum);
			}

			static const VkDescriptorSet nullSet = VK_NULL_HANDLE;
			return nullSet;
		}
	};

}