#pragma once

#include "vkBuffer.h"
#include "vkCommandBuffer.h"
#include "vkGPUProgram.h"
#include <vulkan/vulkan.h>
#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>

namespace vulkan {

	struct GPUQueueFamilyIndices {
		uint32_t graphics;
		uint32_t compute;
		uint32_t transfer;
		uint32_t present;
	};

	enum class GPUQueueFamily : uint8_t {
		F_GRAPHICS = 0,
		F_COMPUTE = 1,
		F_TRANSFER = 2
	};

	class VulkanDevice {
	public:
		VkPhysicalDevice gpu;
		VkPhysicalDeviceProperties gpuProperties;
		VkPhysicalDeviceFeatures gpuFeatures;
		VkPhysicalDeviceMemoryProperties gpuMemoryProperties;
		std::vector<VkQueueFamilyProperties> gpuQueueFamilyProperties;
		std::vector<std::string> supportedExtensions;

		VkDevice device = VK_NULL_HANDLE;
		VkPhysicalDeviceFeatures enabledFeatures;
		GPUQueueFamilyIndices queueFamilyIndices;

		VkCommandPool cmdPools[3];

		explicit VulkanDevice(VkPhysicalDevice physicalDevice);
		~VulkanDevice() {
			destroyBaseCmdPools();
			if (device != VK_NULL_HANDLE) {
				vkDestroyDevice(device, nullptr);
			}
		}

		bool checkPresentSupport(VkSurfaceKHR vkSurface); // check divece support present

		inline bool extensionSupported(std::string extension) const { return (std::find(supportedExtensions.begin(), supportedExtensions.end(), extension) != supportedExtensions.end()); }

		bool checkImageFormatSupported(const VkFormat format) {
			const VkFormatFeatureFlags dstFeatures = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;

			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(gpu, format, &formatProperties);

			return (formatProperties.optimalTilingFeatures & dstFeatures) == dstFeatures;
		}

		VkFormat getSupportedDepthFormat(const uint8_t minBits, bool samplingSupport = false) const {
			constexpr uint8_t formatsCount = 5;
			std::array<VkFormat, formatsCount> depthFormats = { VK_FORMAT_D16_UNORM, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT };
			uint8_t i = 0;
			switch (minBits) {
				case 16:
					break;
				case 24:
					i = 2;
					break;
				case 32: 
					i = 3;
					break;
				default:
					return VK_FORMAT_UNDEFINED;
			}
			for (; i < formatsCount; ++i) {
				auto&& format = depthFormats[i];
				VkFormatProperties formatProperties;
				vkGetPhysicalDeviceFormatProperties(gpu, format, &formatProperties);
				// format must support depth stencil attachment for optimal tiling
				if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
					if (samplingSupport && !(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) { continue; }
					return format;
				}
			}

			return VK_FORMAT_UNDEFINED;
		}

		VkCommandPool createCommandPool(const uint32_t queueFamilyIndex, const VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) const;
		inline void destroyCommandPool(VkCommandPool pool) const {
			if (pool != VK_NULL_HANDLE) {
				vkDestroyCommandPool(device, pool, nullptr);
			}
		}

		inline VkCommandPool getCommandPool(const GPUQueueFamily f) const { return cmdPools[static_cast<uint8_t>(f)]; }
		inline VkCommandPool getCommandPool(const uint32_t queueFamilyIndex) const {
			if (queueFamilyIndex == queueFamilyIndices.graphics) {
				return cmdPools[static_cast<uint8_t>(GPUQueueFamily::F_GRAPHICS)];
			} else if (queueFamilyIndex == queueFamilyIndices.compute) {
				return cmdPools[static_cast<uint8_t>(GPUQueueFamily::F_COMPUTE)];
			} else if (queueFamilyIndex == queueFamilyIndices.transfer) {
				return cmdPools[static_cast<uint8_t>(GPUQueueFamily::F_TRANSFER)];
			}

			return VK_NULL_HANDLE;
		}

		void createCommandBuffers(VkCommandBufferLevel level, VkCommandPool pool, VkCommandBuffer* pCommandBuffers, const uint32_t count) const;

		////
		template<typename STATE = VulkanCommandBufferState>
		VulkanCommandBufferEx<STATE> createVulkanCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool) const { return VulkanCommandBufferEx<STATE>(device, pool, level); }

		template<typename STATE = VulkanCommandBufferState>
		VulkanCommandBuffer createVulkanCommandBuffer(VkCommandBufferLevel level, const GPUQueueFamily f) const { return VulkanCommandBufferEx<STATE>(device, getCommandPool(f), level); }
		////

		// sharing mode must be VK_SHARING_MODE_EXCLUSIVE for only one GPUQueueFamily use or VK_SHARING_MODE_CONCURRENT for other
		VkResult createBuffer(const VkSharingMode sharingMode, const VkBufferUsageFlags usageFlags, const VkMemoryPropertyFlags memoryPropertyFlags, const VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory = nullptr, void* data = nullptr) const;
		void createBuffer(const VkSharingMode sharingMode, const VkBufferUsageFlags usageFlags, const VkMemoryPropertyFlags memoryPropertyFlags, VulkanBuffer* buffer, const VkDeviceSize size) const;
		void createBuffer(const VkSharingMode sharingMode, const VkBufferUsageFlags usageFlags, const VkMemoryPropertyFlags memoryPropertyFlags, VulkanDeviceBuffer* buffer, const VkDeviceSize size0, const VkDeviceSize size1 = VK_WHOLE_SIZE) const;

		inline VkResult waitIdle() const { return vkDeviceWaitIdle(device); }

		uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound = nullptr) const;

		uint32_t getQueueFamilyIndex(VkQueueFlagBits queueFlags) const;
		uint32_t getQueueFamilyCount(VkQueueFlagBits queueFlags) const;

		VkResult createDevice(VkPhysicalDeviceFeatures enabledFeatures, std::vector<const char*> enabledExtensions, void* pNextChain, bool useSwapChain = true, VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

		VkQueue getQueue(const GPUQueueFamily gpuQueue, const uint32_t idx) const;
		VkQueue getPresentQueue() const;

		VkShaderModule createShaderModule(const VulkanShaderCode& code, const VkAllocationCallbacks* pAllocator = nullptr) const;
		void destroyShaderModule(VkShaderModule module, const VkAllocationCallbacks* pAllocator = nullptr) const;

		VkDescriptorSetLayout createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings, const VkAllocationCallbacks* pAllocator = nullptr) const;
		void createDescriptorSetLayout(VkDescriptorSetLayout* setLayout, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, const uint32_t binding, const VkAllocationCallbacks* pAllocator = nullptr) const;
		void destroyDescriptorSetLayout(const VkDescriptorSetLayout setLayout, const VkAllocationCallbacks* pAllocator = nullptr) const;
		void createPipelineLayout(VkPipelineLayout *pipelineLayout, const uint32_t setLayoutCount, const VkDescriptorSetLayout* pSetLayouts, const uint32_t pushConstantRangeCount, const VkPushConstantRange* pPushConstantRanges, VkPipelineLayoutCreateFlags flags = 0, const VkAllocationCallbacks* pAllocator = nullptr) const;
	
		VkRenderPass createRenderPass(const std::vector<VkAttachmentDescription>& attachments, const std::vector<VkSubpassDescription>& subpassDescriptions, const std::vector<VkSubpassDependency>& dependencies, const VkAllocationCallbacks* pAllocator = nullptr) const {
			VkRenderPass renderPass;

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderPassInfo.pAttachments = attachments.data();
			renderPassInfo.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());;
			renderPassInfo.pSubpasses = subpassDescriptions.data();

			if (!dependencies.empty()) {
				renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
				renderPassInfo.pDependencies = dependencies.data();
			}

			vkCreateRenderPass(device, &renderPassInfo, pAllocator, &renderPass);

			return renderPass;
		}

		void destroyRenderPass(VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator = nullptr) const {
			vkDestroyRenderPass(device, renderPass, pAllocator);
		}

	private:
		inline void initBaseCmdPools() {
			cmdPools[static_cast<uint8_t>(GPUQueueFamily::F_GRAPHICS)] = createCommandPool(queueFamilyIndices.graphics);
			cmdPools[static_cast<uint8_t>(GPUQueueFamily::F_COMPUTE)] = ((queueFamilyIndices.compute != queueFamilyIndices.graphics) ?
																			createCommandPool(queueFamilyIndices.compute) : cmdPools[static_cast<uint8_t>(GPUQueueFamily::F_GRAPHICS)]);

			cmdPools[static_cast<uint8_t>(GPUQueueFamily::F_TRANSFER)] = ((queueFamilyIndices.transfer != queueFamilyIndices.graphics) ?
				((queueFamilyIndices.transfer != queueFamilyIndices.compute) ? createCommandPool(queueFamilyIndices.transfer) : cmdPools[static_cast<uint8_t>(GPUQueueFamily::F_COMPUTE)]) : 
					cmdPools[static_cast<uint8_t>(GPUQueueFamily::F_GRAPHICS)]);
			
		}

		inline void destroyBaseCmdPools() {
			destroyCommandPool(cmdPools[static_cast<uint8_t>(GPUQueueFamily::F_GRAPHICS)]);

			if (queueFamilyIndices.compute != queueFamilyIndices.graphics) {
				destroyCommandPool(cmdPools[static_cast<uint8_t>(GPUQueueFamily::F_COMPUTE)]);
			}

			if (queueFamilyIndices.transfer != queueFamilyIndices.graphics && queueFamilyIndices.transfer != queueFamilyIndices.compute) {
				destroyCommandPool(cmdPools[static_cast<uint8_t>(GPUQueueFamily::F_TRANSFER)]);
			}

			cmdPools[static_cast<uint8_t>(GPUQueueFamily::F_GRAPHICS)]	= VK_NULL_HANDLE;
			cmdPools[static_cast<uint8_t>(GPUQueueFamily::F_COMPUTE)]	= VK_NULL_HANDLE;
			cmdPools[static_cast<uint8_t>(GPUQueueFamily::F_TRANSFER)]	= VK_NULL_HANDLE;
		}
	};
}