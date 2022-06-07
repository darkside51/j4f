#include "vkDevice.h"

namespace vulkan {
	VulkanDevice::VulkanDevice(VkPhysicalDevice physicalDevice) {
		gpu = physicalDevice;
		vkGetPhysicalDeviceProperties(physicalDevice, &gpuProperties);
		vkGetPhysicalDeviceFeatures(physicalDevice, &gpuFeatures);
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &gpuMemoryProperties);

		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		gpuQueueFamilyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, gpuQueueFamilyProperties.data());

		// supported extensions
		uint32_t extCount = 0;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
		if (extCount > 0) {
			std::vector<VkExtensionProperties> extensions(extCount);
			if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS) {
				for (auto&& ext : extensions) {
					supportedExtensions.push_back(ext.extensionName);
				}
			}
		}

		cmdPools[0] = VK_NULL_HANDLE;
		cmdPools[1] = VK_NULL_HANDLE;
		cmdPools[2] = VK_NULL_HANDLE;
	}

	bool VulkanDevice::checkPresentSupport(VkSurfaceKHR vkSurface) {
		uint32_t queueNumber = 0;
		for (const VkQueueFamilyProperties& p : gpuQueueFamilyProperties) {
			if (p.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				VkBool32 supportPresent = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(gpu, queueNumber, vkSurface, &supportPresent);
				if (supportPresent) {
					queueFamilyIndices.present = queueNumber;
					return true;
				}
			}
			++queueNumber;
		}

		queueFamilyIndices.present = 0xffffffff;
		return false;
	}

	uint32_t VulkanDevice::getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound) const {
		for (uint32_t i = 0; i < gpuMemoryProperties.memoryTypeCount; ++i) {
			if ((typeBits & 1) == 1) {
				if ((gpuMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
					if (memTypeFound) { *memTypeFound = true; }
					return i;
				}
			}

			typeBits >>= 1;
		}

		if (memTypeFound) {
			*memTypeFound = false;
			return 0;
		} else {
			return 0xffffffff;
		}
	}

	uint32_t VulkanDevice::getQueueFamilyIndex(VkQueueFlagBits queueFlags) const {
		uint32_t i = 0;
		// try to find a queue family index that supports compute but not graphics
		if (queueFlags & VK_QUEUE_COMPUTE_BIT) {
			i = 0;
			for (const VkQueueFamilyProperties& p : gpuQueueFamilyProperties) {
				if ((p.queueFlags & queueFlags) && ((p.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)) { return i; }
				++i;
			}
		}

		// try to find a queue family index that supports transfer but not graphics and compute
		if (queueFlags & VK_QUEUE_TRANSFER_BIT) {
			i = 0;
			for (const VkQueueFamilyProperties& p : gpuQueueFamilyProperties) {
				if ((p.queueFlags & queueFlags) && ((p.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((p.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)) { return i; }
				++i;
			}
		}

		// for other queue types or if no separate compute queue is present, return the first one to support the requested flags
		i = 0;
		for (const VkQueueFamilyProperties& p : gpuQueueFamilyProperties) {
			if (p.queueFlags & queueFlags) { return i; }
			++i;
		}

		return 0xffffffff;
	}

	uint32_t VulkanDevice::getQueueFamilyCount(VkQueueFlagBits queueFlags) const {
		uint32_t count = 0;
		for (const VkQueueFamilyProperties& p : gpuQueueFamilyProperties) {
			if (p.queueFlags & queueFlags) { ++count; }
		}

		return count;
	}

	VkResult VulkanDevice::createDevice(VkPhysicalDeviceFeatures deviceEnabledFeatures, std::vector<const char*> enabledExtensions, void* pNextChain, bool useSwapChain, VkQueueFlags requestedQueueTypes) {
		// due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
		// requests different queue types
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

		// get queue family indices for the requested queue family types
		// note that the indices may overlap depending on the implementation
		const float defaultQueuePriority = 0.0f;

		// graphics queue
		if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT) {
			queueFamilyIndices.graphics = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
			VkDeviceQueueCreateInfo queueInfo;
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.pNext = nullptr;
			queueInfo.flags = 0;
			queueInfo.queueFamilyIndex = queueFamilyIndices.graphics;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			queueCreateInfos.push_back(queueInfo);
		} else {
			queueFamilyIndices.graphics = 0;
		}

		// compute queue
		if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT) {
			queueFamilyIndices.compute = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
			if (queueFamilyIndices.compute != queueFamilyIndices.graphics) {
				// if compute family index differs, we need an additional queue create info for the compute queue
				VkDeviceQueueCreateInfo queueInfo;
				queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfo.pNext = nullptr;
				queueInfo.flags = 0;
				queueInfo.queueFamilyIndex = queueFamilyIndices.compute;
				queueInfo.queueCount = 1;
				queueInfo.pQueuePriorities = &defaultQueuePriority;
				queueCreateInfos.push_back(queueInfo);
			}
		} else {
			// else we use the same queue
			queueFamilyIndices.compute = queueFamilyIndices.graphics;
		}

		// transfer queue
		if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT) {
			queueFamilyIndices.transfer = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
			if ((queueFamilyIndices.transfer != queueFamilyIndices.graphics) && (queueFamilyIndices.transfer != queueFamilyIndices.compute)) {
				// if compute family index differs, we need an additional queue create info for the compute queue
				VkDeviceQueueCreateInfo queueInfo;
				queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfo.pNext = nullptr;
				queueInfo.flags = 0;
				queueInfo.queueFamilyIndex = queueFamilyIndices.transfer;
				queueInfo.queueCount = 1;
				queueInfo.pQueuePriorities = &defaultQueuePriority;
				queueCreateInfos.push_back(queueInfo);
			}
		} else {
			// else we use the same queue
			queueFamilyIndices.transfer = queueFamilyIndices.graphics;
		}

		VkDeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pNext = nullptr;
		deviceCreateInfo.flags = 0;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = &deviceEnabledFeatures;

		deviceCreateInfo.enabledLayerCount = 0;
		deviceCreateInfo.ppEnabledLayerNames = nullptr;

		// if a pNext(Chain) has been passed, we need to add it to the device creation info
		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2;
		if (pNextChain) {
			physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			physicalDeviceFeatures2.features = deviceEnabledFeatures;
			physicalDeviceFeatures2.pNext = pNextChain;
			deviceCreateInfo.pEnabledFeatures = nullptr;
			deviceCreateInfo.pNext = &physicalDeviceFeatures2;
		}

		// create the logical device representation
		std::vector<const char*> deviceExtensions(enabledExtensions);
		if (useSwapChain) {
			// if the device will be used for presenting to a display via a swapchain we need to request the swapchain extension
			deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		if (!deviceExtensions.empty()) {
			for (const char* enabledExtension : deviceExtensions) { // check supported exts
				if (!extensionSupported(enabledExtension)) {
					//std::cerr << "Enabled device extension \"" << enabledExtension << "\" is not present at device level\n";
				}
			}

			deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		}

		enabledFeatures = deviceEnabledFeatures;

		VkResult result = vkCreateDevice(gpu, &deviceCreateInfo, nullptr, &device);
		if (result != VK_SUCCESS) { return result; }

		initBaseCmdPools();

		return VkResult::VK_SUCCESS;
	}

	VkCommandPool VulkanDevice::createCommandPool(const uint32_t queueFamilyIndex, const VkCommandPoolCreateFlags createFlags) const {
		VkCommandPoolCreateInfo cmdPoolInfo;
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.pNext = nullptr;
		cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
		cmdPoolInfo.flags = createFlags;
		VkCommandPool cmdPool;
		vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool);
		return cmdPool;
	}

	void VulkanDevice::createCommandBuffers(VkCommandBufferLevel level, VkCommandPool pool, VkCommandBuffer* pCommandBuffers, const uint32_t count) const {
		VkCommandBufferAllocateInfo cmdBufAllocateInfo;
		cmdBufAllocateInfo.pNext = nullptr;
		cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocateInfo.commandPool = pool;
		cmdBufAllocateInfo.level = level;
		cmdBufAllocateInfo.commandBufferCount = count;

		vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, pCommandBuffers);
	}

	VkResult VulkanDevice::createBuffer(const VkSharingMode sharingMode, const VkBufferUsageFlags usageFlags, const VkMemoryPropertyFlags memoryPropertyFlags, const VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory, void* data) const {
		// create the buffer handle
		VkBufferCreateInfo bufferCreateInfo;
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.sharingMode = sharingMode;
		bufferCreateInfo.usage = usageFlags;
		bufferCreateInfo.size = size;
		bufferCreateInfo.pNext = nullptr;
		bufferCreateInfo.flags = 0;
		vkCreateBuffer(device, &bufferCreateInfo, nullptr, buffer);

		// create the memory backing up the buffer handle
		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(device, *buffer, &memReqs);
		// find a memory type index that fits the properties of the buffer
		const uint32_t memoryTypeIdx = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);

		VkMemoryAllocateInfo memAlloc;
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = memoryTypeIdx;
		// if the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation
		VkMemoryAllocateFlagsInfoKHR allocFlagsInfo;
		if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
			allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
			allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
			memAlloc.pNext = &allocFlagsInfo;
		} else {
			memAlloc.pNext = nullptr;
		}
		vkAllocateMemory(device, &memAlloc, nullptr, memory);

		// if a pointer to the buffer data has been passed, map the buffer and copy over the data
		if (data != nullptr) {
			void* mapped;
			vkMapMemory(device, *memory, 0, size, 0, &mapped);
			memcpy(mapped, data, size);
			// if host coherency hasn't been requested, do a manual flush to make writes visible
			if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
				VkMappedMemoryRange mappedRange;
				mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				mappedRange.memory = *memory;
				mappedRange.offset = 0;
				mappedRange.size = size;
				vkFlushMappedMemoryRanges(device, 1, &mappedRange);
			}
			vkUnmapMemory(device, *memory);
		}

		// attach the memory to the buffer object
		vkBindBufferMemory(device, *buffer, *memory, 0);

		return VkResult::VK_SUCCESS;
	}

	void VulkanDevice::createBuffer(const VkSharingMode sharingMode, const VkBufferUsageFlags usageFlags, const VkMemoryPropertyFlags memoryPropertyFlags, VulkanBuffer* buffer, const VkDeviceSize size) const {
		VkBufferCreateInfo bufferCreateInfo;
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.sharingMode = sharingMode;
		bufferCreateInfo.usage = usageFlags;
		bufferCreateInfo.size = size;
		bufferCreateInfo.pNext = nullptr;
		bufferCreateInfo.flags = 0;
		vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer->m_buffer);

		// create the memory backing up the buffer handle
		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(device, buffer->m_buffer, &memReqs);
		// find a memory type index that fits the properties of the buffer
		const uint32_t memoryTypeIdx = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);

		VkMemoryAllocateInfo memAlloc;
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = memoryTypeIdx;

		// If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation
		VkMemoryAllocateFlagsInfoKHR allocFlagsInfo;
		if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
			allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
			allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
			memAlloc.pNext = &allocFlagsInfo;
		} else {
			memAlloc.pNext = nullptr;
		}

		vkAllocateMemory(device, &memAlloc, nullptr, &buffer->m_memory);

		buffer->m_device = device;
		buffer->m_usage = usageFlags;
		buffer->m_properties = memoryPropertyFlags;
		buffer->m_size = size;
		buffer->m_alignment = memReqs.alignment;

		// attach the memory to the buffer object
		vkBindBufferMemory(device, buffer->m_buffer, buffer->m_memory, 0);
	}

	void VulkanDevice::createBuffer(const VkSharingMode sharingMode, const VkBufferUsageFlags usageFlags, const VkMemoryPropertyFlags memoryPropertyFlags, VulkanDeviceBuffer* buffer, const VkDeviceSize size0, const VkDeviceSize size1) const {
		createBuffer(
			sharingMode,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&buffer->m_stageBuffer,
			size0
		);

		createBuffer(
			sharingMode,
			usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			memoryPropertyFlags | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&buffer->m_gpuBuffer,
			(size1 == VK_WHOLE_SIZE ? size0 : size1)
		);
	}

	VkQueue VulkanDevice::getQueue(const GPUQueueFamily gpuQueue, const uint32_t idx) const {
		VkQueue queue;
		switch (gpuQueue) {
		case GPUQueueFamily::F_GRAPHICS:
			vkGetDeviceQueue(device, queueFamilyIndices.graphics, idx, &queue);
			break;
		case GPUQueueFamily::F_COMPUTE:
			vkGetDeviceQueue(device, queueFamilyIndices.compute, idx, &queue);
			break;
		case GPUQueueFamily::F_TRANSFER:
			vkGetDeviceQueue(device, queueFamilyIndices.transfer, idx, &queue);
			break;
		default: 
			vkGetDeviceQueue(device, queueFamilyIndices.graphics, idx, &queue);
			break;
		}

		return queue;
	}

	VkQueue VulkanDevice::getPresentQueue() const {
		if (queueFamilyIndices.present == queueFamilyIndices.graphics) {
			VkQueue queue;
			vkGetDeviceQueue(device, queueFamilyIndices.graphics, 0, &queue);
			return queue;
		}

		return VK_NULL_HANDLE;
	}

	VkShaderModule VulkanDevice::createShaderModule(const VulkanShaderCode& code, const VkAllocationCallbacks* pAllocator) const {
		VkShaderModuleCreateInfo moduleCreateInfo;
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.flags = 0;
		moduleCreateInfo.pNext = nullptr;
		moduleCreateInfo.codeSize = code.shaderSize;
		moduleCreateInfo.pCode = reinterpret_cast<uint32_t*>(code.shaderCode);

		VkShaderModule shaderModule;
		vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &shaderModule);
		return shaderModule;
	}

	void VulkanDevice::destroyShaderModule(VkShaderModule module, const VkAllocationCallbacks* pAllocator) const {
		vkDestroyShaderModule(device, module, pAllocator);
	}

	VkDescriptorSetLayout VulkanDevice::createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings, const VkAllocationCallbacks* pAllocator) const {
		if (bindings.empty()) return VK_NULL_HANDLE;

		VkDescriptorSetLayout setLayout;

		VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
		descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayout.pNext = nullptr;
		descriptorLayout.bindingCount = static_cast<uint32_t>(bindings.size());
		descriptorLayout.pBindings = bindings.data();

		vkCreateDescriptorSetLayout(device, &descriptorLayout, pAllocator, &setLayout);

		return setLayout;
	}

	void VulkanDevice::createDescriptorSetLayout(VkDescriptorSetLayout* setLayout, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags,
												const uint32_t binding, const VkAllocationCallbacks* pAllocator) const {
		VkDescriptorSetLayoutBinding layoutBinding;
		layoutBinding.binding = binding;
		layoutBinding.descriptorType = descriptorType;
		layoutBinding.descriptorCount = 1;
		layoutBinding.stageFlags = stageFlags;
		layoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
		descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayout.pNext = nullptr;
		descriptorLayout.bindingCount = 1;
		descriptorLayout.pBindings = &layoutBinding;

		vkCreateDescriptorSetLayout(device, &descriptorLayout, pAllocator, setLayout);
	}

	void VulkanDevice::destroyDescriptorSetLayout(const VkDescriptorSetLayout setLayout, const VkAllocationCallbacks* pAllocator) const {
		vkDestroyDescriptorSetLayout(device, setLayout, pAllocator);
	}

	 void VulkanDevice::createPipelineLayout(VkPipelineLayout* pipelineLayout, const uint32_t setLayoutCount, const VkDescriptorSetLayout* pSetLayouts, const uint32_t pushConstantRangeCount,
											const VkPushConstantRange* pPushConstantRanges, VkPipelineLayoutCreateFlags flags, const VkAllocationCallbacks* pAllocator) const {
		 VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo;
		 pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		 pPipelineLayoutCreateInfo.pNext = nullptr;
		 pPipelineLayoutCreateInfo.flags = flags;
		 pPipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
		 pPipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;
		 pPipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRangeCount;
		 pPipelineLayoutCreateInfo.pPushConstantRanges = pPushConstantRanges;

		 vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, pAllocator, pipelineLayout);
	 }

}