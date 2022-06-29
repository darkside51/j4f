#include "vkRenderer.h"
#include "vkHelper.h"

#include "../RenderSurfaceInitialisez.h"
#include "vkTexture.h"

#include <unordered_map>
#include <unordered_set>
#include <cassert>

#ifdef VK_USE_PLATFORM_WIN32_KHR
#define VULKAN_PLATFORM_SURFACE_EXT VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif // VK_USE_PLATFORM_WIN32_KHR

#define EXECUTE_PROC_OR_NULLIFY_AND_RETURN(x, proc, ...) if (!x->proc(__VA_ARGS__)) { delete x; return nullptr; }

namespace vulkan {

// ---------- initialisation ----------
	VulkanRenderer::VulkanRenderer() : _width(1024), _height(768), _vSync(false) { }

	VulkanRenderer::~VulkanRenderer() {
		destroy();
	}

	bool VulkanRenderer::createInstance(const std::vector<VkExtensionProperties>& desiredInstanceExtensions, const std::vector<VkLayerProperties>& desiredLayers) {

		VkApplicationInfo vk_applicationInfo;
		vk_applicationInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_APPLICATION_INFO;
		vk_applicationInfo.pNext = nullptr;
		vk_applicationInfo.pApplicationName = "j4f_application";
		vk_applicationInfo.applicationVersion = 1;
		vk_applicationInfo.pEngineName = "j4f_engine";
		vk_applicationInfo.engineVersion = 1;
		vk_applicationInfo.apiVersion = VK_MAKE_VERSION(1, 2, 182);

		std::vector<VkExtensionProperties> supportedExtensions;
		getVulkanInstanceSupportedExtensions(supportedExtensions);

		std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME, VULKAN_PLATFORM_SURFACE_EXT };

		std::vector<VkLayerProperties> supportedLayers;
		getVulkanInstanceSupportedLayers(supportedLayers);

		std::vector<const char*> instancelayers;

#ifdef _DEBUG
		const bool validationEnable = true;
#else
		const bool validationEnable = false;
#endif

		auto addInstanceExtension = [&instanceExtensions, &supportedExtensions](const char* extName) {
			if (std::find_if(supportedExtensions.begin(), supportedExtensions.end(), [extName](const VkExtensionProperties& ext) { return strcmp(ext.extensionName, extName) == 0; }) != supportedExtensions.end()) {
				instanceExtensions.push_back(extName);
			}
		};

		auto addInstanceLayer = [&instancelayers, &supportedLayers](const char* layerName) {
			if (std::find_if(supportedLayers.begin(), supportedLayers.end(), [layerName](const VkLayerProperties& p) { return strcmp(p.layerName, layerName) == 0; }) != supportedLayers.end()) {
				instancelayers.push_back(layerName);
			}
		};

		if (validationEnable) {
			addInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			addInstanceLayer("VK_LAYER_KHRONOS_validation");
		}

		if (!desiredInstanceExtensions.empty()) {
			for (auto&& ext : desiredInstanceExtensions) {
				addInstanceExtension(ext.extensionName);
			}
		}

		if (!desiredLayers.empty()) {
			for (auto&& layer : desiredLayers) {
				addInstanceLayer(layer.layerName);
			}
		}

		VkInstanceCreateInfo vk_instanceCreateInfo;
		vk_instanceCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		vk_instanceCreateInfo.pNext = nullptr;
		vk_instanceCreateInfo.flags = 0;
		vk_instanceCreateInfo.pApplicationInfo = &vk_applicationInfo;
		vk_instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(instancelayers.size());
		vk_instanceCreateInfo.ppEnabledLayerNames = instancelayers.data();
		vk_instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
		vk_instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

		VkResult result = vkCreateInstance(&vk_instanceCreateInfo, nullptr, &_instance);
		return result == VkResult::VK_SUCCESS;
	}

	bool VulkanRenderer::createDevice(const VkPhysicalDeviceFeatures features, std::vector<const char*>& extensions, const VkPhysicalDeviceType deviceType) {
		uint32_t vk_physicalDevicesCount;
		if (vkEnumeratePhysicalDevices(_instance, &vk_physicalDevicesCount, nullptr) != VkResult::VK_SUCCESS) { return false; }

		std::vector<VkPhysicalDevice> vk_physicalDevices(vk_physicalDevicesCount);
		if (vkEnumeratePhysicalDevices(_instance, &vk_physicalDevicesCount, &vk_physicalDevices[0]) != VkResult::VK_SUCCESS) { return false; }

		if (vk_physicalDevices.empty()) { return false; }

		extensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME); // for negative viewPort height

		auto checkGPU = [features, &extensions](vulkan::VulkanDevice* device) -> bool {
			// device->gpuFeatures check (some hack :( )	
			const uint32_t featuresCount = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
			for (uint8_t i = 0; i < featuresCount; ++i) {
				const VkBool32 support_feature = *(reinterpret_cast<const VkBool32*>(&(features)) + i);
				const VkBool32 device_support_feature = *(reinterpret_cast<const VkBool32*>(&(device->gpuFeatures)) + i);
				if (device_support_feature < support_feature) {
					return false;
				}
			}

			// device->supportedExtensions check
			for (const char* ext : extensions) {
				if (std::find(device->supportedExtensions.begin(), device->supportedExtensions.end(), ext) == device->supportedExtensions.end()) {
					return false;
				}
			}
			return true;
		};

		if (vk_physicalDevicesCount > 1) {
			for (VkPhysicalDevice gpu : vk_physicalDevices) {
				// check support features + extensions 
				// get desired deviceType if exist and it ok
				vulkan::VulkanDevice* condidate = new vulkan::VulkanDevice(gpu);
				if (checkGPU(condidate)) {
					if (_vulkanDevice) {
						delete _vulkanDevice;
						_vulkanDevice = nullptr;
					}
					_vulkanDevice = condidate;
					if (condidate->gpuProperties.deviceType == deviceType) {
						break;
					}
				} else {
					delete condidate;
				}
			}
		} else {
			vulkan::VulkanDevice* condidate = new vulkan::VulkanDevice(vk_physicalDevices[0]);
			if (checkGPU(condidate)) {
				_vulkanDevice = condidate;
			} else {
				delete condidate;
			}
		}

		if (_vulkanDevice == nullptr) { // error no device with supported features + extensions
			return false;
		}

		_useSharedMemory =_vulkanDevice->gpuProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

		const VkResult res = _vulkanDevice->createDevice(features, extensions, _deviceCreatepNextChain);
		return res == VK_SUCCESS;
	}

	void VulkanRenderer::createSwapChain(const engine::IRenderSurfaceInitialiser* initialiser, const bool useVsync) {
		_vSync = useVsync;
		_swapChain.connect(_instance, _vulkanDevice, initialiser);
		_swapChain.resize(_width, _height, useVsync);
	}

	void VulkanRenderer::init() {
		_swapchainImagesCount = _swapChain.imageCount;

		// get a graphics & present queue from the device (its eaqual i think)
		_mainQueue = _vulkanDevice->getQueue(vulkan::GPUQueueFamily::F_GRAPHICS, 0);
		_presentQueue = _vulkanDevice->getPresentQueue();

		// createCommandPool
		_commandPool = _vulkanDevice->getCommandPool(vulkan::GPUQueueFamily::F_GRAPHICS);

		// createCommandBuffers
		_mainRenderCommandBuffers = VulkanCommandBuffersArray(_vulkanDevice->device, _commandPool, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, _swapchainImagesCount);
		_mainSupportCommandBuffers = VulkanCommandBuffersArray(_vulkanDevice->device, _commandPool, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, _swapchainImagesCount);

		// create synchronization objects
		// create a semaphore used to synchronize image presentation
		// ensures that the image is displayed before we start submitting new commands to the queue
		_presentCompleteSemaphores.reserve(_swapchainImagesCount);
		for (size_t i = 0; i < _swapchainImagesCount; ++i) {
			_presentCompleteSemaphores.emplace_back(vulkan::VulkanSemaphore(_vulkanDevice->device));
			_mainSupportCommandBuffers.addWaitSemaphore(_presentCompleteSemaphores[i].semaphore, i);
		}

		_mainRenderCommandBuffers.depend(_mainSupportCommandBuffers);

		// wait fences to sync command buffer access
		for (uint32_t i = 0; i < _swapchainImagesCount; ++i) {
			_waitFences.emplace_back(vulkan::VulkanFence(_vulkanDevice->device, VK_FENCE_CREATE_SIGNALED_BIT));
		}

		// find a suitable depth format
		_mainDepthFormat = _vulkanDevice->getSupportedDepthFormat();

		// setupDepthStencil
		_depthStencil = vulkan::VulkanImage(_vulkanDevice, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TYPE_2D, _mainDepthFormat, 1, _width, _height);
		_depthStencil.createImageView();

		// createPipelineCache
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		vkCreatePipelineCache(_vulkanDevice->device, &pipelineCacheCreateInfo, nullptr, &_pipelineCache);

		// setupRenderPass
		setupRenderPass({}, {}); // todo: VkSubpassDependency parametrisation

		// setupFrameBuffers
		std::vector<VkImageView> attachments(2);
		attachments[1] = _depthStencil.view; // depth/stencil attachment is the same for all frame buffers

		_frameBuffers.reserve(_swapchainImagesCount);
		for (size_t i = 0; i < _swapchainImagesCount; ++i) {
			attachments[0] = _swapChain.images[i].view; // color attachment is the view of the swapchain image
			_frameBuffers.emplace_back(_vulkanDevice, _width, _height, 1, _mainRenderPass, attachments.data(), attachments.size());
		}

		// setupDescriptorPool
		setupDescriptorPool({}); // todo: descriptor pool parametrisation

		// clear values setup
		_mainClearValues[0].color = { 0.5f, 0.5f, 0.5f, 1.0f };
		_mainClearValues[1].depthStencil = { 1.0f, 0 };

		///////////////////////////////// default fill for _mainRenderCommandBuffers
		buildDefaultMainRenderCommandBuffer();
		//////////////////////////////////

		_tmpBuffers.resize(_swapchainImagesCount);

		createEmtyTexture();
	}

	void VulkanRenderer::createEmtyTexture() {
#ifdef _DEBUG
		_emptyTexture = new VulkanTexture(this, 2, 2, 1);
		const unsigned char emptyImg[16] = { 
			100, 100, 100, 255, 160, 160, 160, 255,
			160, 160, 160, 255, 100, 100, 100, 255
	};
#else
		_emptyTexture = new VulkanTexture(this, 1, 1, 1);
		const unsigned char emptyImg[4] = { 200, 200, 200, 255 };

		/*_emptyTexture = new VulkanTexture(this, 2, 2, 1);
		const unsigned char emptyImg[16] = { 
			100, 100, 100, 255, 160, 160, 160, 255,
			160, 160, 160, 255, 100, 100, 100, 255
		};*/

#endif

		_emptyTexture->setSampler(getSampler(
			VK_FILTER_NEAREST,
			VK_FILTER_NEAREST,
			VK_SAMPLER_MIPMAP_MODE_NEAREST,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK
		));

		_emptyTexture->create(&emptyImg[0], VK_FORMAT_R8G8B8A8_UNORM, 32, false, false);
		_emptyTexture->createSingleDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0);
	}

	VkSampler VulkanRenderer::getSampler(
		const VkFilter minFilter,
		const VkFilter magFilter,
		const VkSamplerMipmapMode mipmapMode,
		const VkSamplerAddressMode addressModeU,
		const VkSamplerAddressMode addressModeV,
		const VkSamplerAddressMode addressModeW,
		const VkBorderColor borderColor
	) {
		uint16_t samplerDescription = 0;
		// filtration
		samplerDescription |= minFilter << 0; // 1 bit
		samplerDescription |= magFilter << 1; // 1 bit
		samplerDescription |= mipmapMode << 2; // 1 bit
		// addresModes
		samplerDescription |= addressModeU << 3; // 3 bits
		samplerDescription |= addressModeV << 6; // 3 bits
		samplerDescription |= addressModeW << 9; // 3 bits
		samplerDescription |= borderColor << 12; // 3 bits

		auto it = _samplers.find(samplerDescription);
		if (it != _samplers.end()) {
			return it->second;
		}

		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.pNext = nullptr;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = VK_REMAINING_MIP_LEVELS;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0;
		samplerInfo.borderColor = borderColor;

		samplerInfo.minFilter = minFilter;
		samplerInfo.magFilter = magFilter;
		samplerInfo.mipmapMode = mipmapMode;
		samplerInfo.addressModeU = addressModeU;
		samplerInfo.addressModeV = addressModeV;
		samplerInfo.addressModeW = addressModeW;

		VkSampler sampler;
		vkCreateSampler(_vulkanDevice->device, &samplerInfo, nullptr, &sampler);
		_samplers[samplerDescription] = sampler;
		return sampler;
	}

	void VulkanRenderer::buildDefaultMainRenderCommandBuffer() {
		for (uint32_t i = 0; i < _swapchainImagesCount; ++i) {
			VulkanCommandBuffer& commandBuffer = _mainRenderCommandBuffers[i];

			commandBuffer.begin();

			// Start the first sub pass specified in our default render pass setup by the base class
			// This will clear the color and depth attachment
			VkRenderPassBeginInfo renderPassBeginInfo = createRenderPassBeginInfo(_mainRenderPass, { {0, 0}, {_width, _height} }, &_mainClearValues[0], 2, _frameBuffers[i].m_framebuffer);
			commandBuffer.cmdBeginRenderPass(renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Update dynamic viewport state
			commandBuffer.cmdSetViewport(0.0f, 0.0f, static_cast<float>(_width), static_cast<float>(_height));

			// Update dynamic scissor state
			commandBuffer.cmdSetScissor(0, 0, _width, _height);

			///////////////// finish cmd buffer
			commandBuffer.cmdEndRenderPass();

			// Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to
			// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system
			commandBuffer.end();
		}
	}

	void VulkanRenderer::setupRenderPass(const std::vector<VkAttachmentDescription>& configuredAttachments, const std::vector<VkSubpassDependency>& configuredDependencies) {

		auto createDefaultAttachments = [this]()->std::vector<VkAttachmentDescription> {
			std::vector<VkAttachmentDescription> attachments(2);
			// color attachment
			attachments[0] = vulkan::createAttachmentDescription(_swapChain.colorFormat, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
			// depth attachment
			attachments[1] = vulkan::createAttachmentDescription(_mainDepthFormat, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);
			return attachments;
		};

		const std::vector<VkAttachmentDescription>& attachments = configuredAttachments.empty() ? createDefaultAttachments () : configuredAttachments;

		if (configuredDependencies.empty()) {
			// subpass dependencies for layout transitions
			std::vector<VkSubpassDependency> dependencies(2);

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;                             // Producer of the dependency
			dependencies[0].dstSubpass = 0;                                               // Consumer is our single subpass that will wait for the execution dependency
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Match our pWaitDstStageMask when we vkQueueSubmit
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // is a loadOp stage for color attachments
			dependencies[0].srcAccessMask = 0;                                            // semaphore wait already does memory dependency for us
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;         // is a loadOp CLEAR access mask for color attachments
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			// Second dependency at the end the renderpass
			// Does the transition from the initial to the final layout
			// Technically this is the same as the implicit subpass dependency, but we are gonna state it explicitly here
			dependencies[1].srcSubpass = 0;                                               // Producer of the dependency is our single subpass
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;                             // Consumer are all commands outside of the renderpass
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // is a storeOp stage for color attachments
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;          // Do not block any subsequent work
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;         // is a storeOp `STORE` access mask for color attachments
			dependencies[1].dstAccessMask = 0;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			_mainRenderPass = createRenderPass(_vulkanDevice, VK_PIPELINE_BIND_POINT_GRAPHICS, dependencies, attachments);
		} else {
			_mainRenderPass = createRenderPass(_vulkanDevice, VK_PIPELINE_BIND_POINT_GRAPHICS, configuredDependencies, attachments);
		}
	}

	void VulkanRenderer::setupDescriptorPool(const std::vector<VkDescriptorPoolSize>& descriptorPoolCfg) {
		//https://habr.com/ru/post/584554/

		// Create the global descriptor pool
		// All descriptors used in this example are allocated from this pool
		VkDescriptorPoolCreateInfo descriptorPoolInfo;
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.pNext = nullptr;
		descriptorPoolInfo.poolSizeCount = 5;
		descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		// Set the max. number of descriptor sets that can be requested from this pool (requesting beyond this limit will result in an error)
		descriptorPoolInfo.maxSets = 2048;

		if (descriptorPoolCfg.empty()) { // default variant
			// we need to tell the API the number of max. requested descriptors per type
			VkDescriptorPoolSize typeCounts[5];
			typeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			typeCounts[0].descriptorCount = 128;

			typeCounts[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			typeCounts[1].descriptorCount = 1024;

			typeCounts[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			typeCounts[2].descriptorCount = 16;

			typeCounts[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			typeCounts[3].descriptorCount = 128;

			typeCounts[4].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			typeCounts[4].descriptorCount = 2048;

			descriptorPoolInfo.pPoolSizes = &typeCounts[0];
			vkCreateDescriptorPool(_vulkanDevice->device, &descriptorPoolInfo, nullptr, &_globalDescriptorPool);
		} else {
			descriptorPoolInfo.pPoolSizes = &descriptorPoolCfg[0];
			vkCreateDescriptorPool(_vulkanDevice->device, &descriptorPoolInfo, nullptr, &_globalDescriptorPool);
		}

	}

	PipelineDescriptorLayout& VulkanRenderer::getDescriptorLayout(const std::vector<std::vector<VkDescriptorSetLayoutBinding*>>& setLayoutBindings, const std::vector<VkPushConstantRange*>& pushConstantsRanges) {
		// todo: need multithreading synchronisation
		const size_t constantsCount = pushConstantsRanges.size();
		std::vector<uint64_t> constantsCharacterVec(constantsCount);

		for (size_t i = 0; i < constantsCount; ++i) {
			const uint8_t stageFlags = static_cast<uint8_t>(pushConstantsRanges[i]->stageFlags);
			uint16_t offset = pushConstantsRanges[i]->offset;
			uint16_t size = pushConstantsRanges[i]->size;
			const uint64_t characteristic = static_cast<uint64_t>(stageFlags) << 0 |
											static_cast<uint64_t>(offset) << 16 |
											static_cast<uint64_t>(size) << 32;
			constantsCharacterVec[i] = characteristic;
		}

		const size_t bindingsSetsCount = setLayoutBindings.size();
		std::vector<std::vector<uint32_t>> bindingsCharacterVec(bindingsSetsCount);

		for (size_t j = 0; j < bindingsSetsCount; ++j) {
			const std::vector<VkDescriptorSetLayoutBinding*>& bindings = setLayoutBindings[j];
			const size_t bindingsCount = bindings.size();
			bindingsCharacterVec[j].resize(bindingsCount);

			for (size_t i = 0; i < bindingsCount; ++i) {
				const uint8_t binding = bindings[i]->binding;
				const uint8_t descriptorType = static_cast<uint8_t>(bindings[i]->descriptorType);
				const uint8_t descriptorCount = bindings[i]->descriptorCount;
				const uint8_t stageFlags = static_cast<uint8_t>(bindings[i]->stageFlags);
				const uint32_t characteristic = static_cast<uint32_t>(binding) << 0 |
												static_cast<uint32_t>(descriptorType) << 8 |
												static_cast<uint32_t>(descriptorCount) << 16 |
												static_cast<uint32_t>(stageFlags) << 24;
				bindingsCharacterVec[j][i] = characteristic;
			}
		}

		//
		const bool isEmptyLayoutBindings = setLayoutBindings.empty();
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts(bindingsSetsCount);
		uint16_t descriptorSetLayoutsHandled = 0;

		if (!isEmptyLayoutBindings) {
			memset(&descriptorSetLayouts[0], 0, bindingsSetsCount * sizeof(VkDescriptorSetLayout));
		}

		if (isEmptyLayoutBindings) {
			for (auto&& v : _descriptorLayoutsCache) {
				if (v->constantsCharacterVec == constantsCharacterVec) return v->layout;
			}
		} else {
			for (auto&& v : _descriptorLayoutsCache) {
				if (v->bindingsCharacterVec == bindingsCharacterVec) {
					if (v->constantsCharacterVec == constantsCharacterVec) { return v->layout; }
					descriptorSetLayouts = v->layout.descriptorSetLayouts; // if uniforms description exist
				} else { // частичное присваивание биндингов
					size_t j = 0;
					for (std::vector<uint32_t>& l : v->bindingsCharacterVec) {
						for (size_t i = 0; i < bindingsSetsCount; ++i) {
							if (l == bindingsCharacterVec[i]) {
								descriptorSetLayouts[i] = v->layout.descriptorSetLayouts[j];
							}
						}
						++j;
					}
				}
			}
		}

		size_t i = 0;
		for (VkDescriptorSetLayout& descriptorSetLayout : descriptorSetLayouts) {  // попробуем с временным копированием, чтоб не хранить копии постоянно
			if (descriptorSetLayout == VK_NULL_HANDLE) {
				const size_t sz = setLayoutBindings[i].size();
				std::vector<VkDescriptorSetLayoutBinding> bindings(sz);
				for (size_t j = 0; j < sz; ++j) {
					bindings[j] = *(setLayoutBindings[i][j]);
				}
				descriptorSetLayout = _vulkanDevice->createDescriptorSetLayout(bindings, nullptr);
				descriptorSetLayoutsHandled |= (1 << i);
			}
			++i;
		}

		std::vector<VkPushConstantRange> pushConstants(constantsCount); // попробуем с временным копированием, чтоб не хранить копии постоянно
		for (size_t i = 0; i < constantsCount; ++i) {
			pushConstants[i] = *(pushConstantsRanges[i]);
		}

		VkPipelineLayout pipelineLayout;
		if (isEmptyLayoutBindings) {
			_vulkanDevice->createPipelineLayout(&pipelineLayout, 0, VK_NULL_HANDLE, constantsCount, &pushConstants[0]);
		} else {
			_vulkanDevice->createPipelineLayout(&pipelineLayout, descriptorSetLayouts.size(), &descriptorSetLayouts[0], constantsCount, &pushConstants[0]);
		}

		CachedDescriptorLayouts* layout = new CachedDescriptorLayouts(std::move(bindingsCharacterVec), std::move(constantsCharacterVec), { pipelineLayout, descriptorSetLayouts }, descriptorSetLayoutsHandled);
		_descriptorLayoutsCache.push_back(layout);
		return layout->layout;
	}


	VulkanDescriptorSet* VulkanRenderer::allocateDescriptorSetFromGlobalPool(const VkDescriptorSetLayout descriptorSetLayout, const uint32_t count) {
		const uint32_t setsCount = count == 0 ? _swapchainImagesCount : count;
	
		std::vector<VkDescriptorSetLayout> layouts(setsCount, descriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.descriptorPool = _globalDescriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
		allocInfo.pSetLayouts = &layouts[0];

		VulkanDescriptorSet* descriptorSet = new VulkanDescriptorSet(setsCount);
		vkAllocateDescriptorSets(_vulkanDevice->device, &allocInfo, &descriptorSet->set[0]);
		descriptorSet->parentPool = _globalDescriptorPool;

		return descriptorSet;
	}

	VkDescriptorSet VulkanRenderer::allocateSingleDescriptorSetFromGlobalPool(const VkDescriptorSetLayout descriptorSetLayout) {
		VkDescriptorSetAllocateInfo allocInfo;
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.descriptorPool = _globalDescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		VkDescriptorSet descriptorSet;
		vkAllocateDescriptorSets(_vulkanDevice->device, &allocInfo, &descriptorSet);
		
		return descriptorSet;
	}

	void VulkanRenderer::bindBufferToDescriptorSet(
		const VulkanDescriptorSet* descriptorSet,
		const VkDescriptorType type, 
		const VulkanBuffer* buffer,
		const uint32_t binding,
		const uint32_t alignedSize,
		const uint32_t offset
	) const {
		const uint32_t setsCount = static_cast<uint32_t>(descriptorSet->set.size());
		std::vector<VkWriteDescriptorSet> writeDescriptorSet(setsCount);

		for (size_t i = 0; i < setsCount; ++i) {
			VkDescriptorBufferInfo bufferDescriptor = { buffer[i].m_buffer, offset, alignedSize };
			writeDescriptorSet[i] = vulkan::initWriteDescriptorSet(type, 1, binding, descriptorSet->set[i], &bufferDescriptor);
			vkUpdateDescriptorSets(_vulkanDevice->device, 1, &writeDescriptorSet[i], 0, nullptr);
		}
	}

	void VulkanRenderer::bindImageToDescriptorSet(
		const VulkanDescriptorSet* descriptorSet,
		const VkDescriptorType type,
		const VkSampler sampler,
		const VkImageView imageView,
		const VkImageLayout imageLayout,
		const uint32_t binding
	) const {
		const uint32_t setsCount = static_cast<uint32_t>(descriptorSet->set.size());
		std::vector<VkWriteDescriptorSet> writeDescriptorSet(setsCount);

		VkDescriptorImageInfo textureDescriptor;
		textureDescriptor.imageView = imageView;									// The image's view (images are never directly accessed by the shader, but rather through views defining subresources)
		textureDescriptor.sampler = sampler;										// The sampler (Telling the pipeline how to sample the texture, including repeat, border, etc.)
		textureDescriptor.imageLayout = imageLayout;

		for (size_t i = 0; i < setsCount; ++i) {
			writeDescriptorSet[i] = vulkan::initWriteDescriptorSet(type, 1, binding, descriptorSet->set[i], &textureDescriptor);
			vkUpdateDescriptorSets(_vulkanDevice->device, 1, &writeDescriptorSet[i], 0, nullptr);
		}
	}

	void VulkanRenderer::bindImageToSingleDescriptorSet(
		const VkDescriptorSet descriptorSet,
		const VkDescriptorType type,
		const VkSampler sampler,
		const VkImageView imageView,
		const VkImageLayout imageLayout,
		const uint32_t binding
	) const {
		VkDescriptorImageInfo textureDescriptor;
		textureDescriptor.imageView = imageView;									// The image's view (images are never directly accessed by the shader, but rather through views defining subresources)
		textureDescriptor.sampler = sampler;										// The sampler (Telling the pipeline how to sample the texture, including repeat, border, etc.)
		textureDescriptor.imageLayout = imageLayout;

		VkWriteDescriptorSet writeDescriptorSet = vulkan::initWriteDescriptorSet(type, 1, binding, descriptorSet, &textureDescriptor);
		vkUpdateDescriptorSets(_vulkanDevice->device, 1, &writeDescriptorSet, 0, nullptr);
	}

	void VulkanRenderer::resize(const uint32_t w, const uint32_t h, const bool vSync) {
		if (_width != w || _height != h || _vSync != vSync) {
			_width = w; _height = h; _vSync = vSync;

			vkQueueWaitIdle(_mainQueue);
			vkQueueWaitIdle(_presentQueue);

			_vulkanDevice->waitIdle();

			_waitFences.clear();

			for (auto&& semaphore : _presentCompleteSemaphores) {
				semaphore.destroy();
			}
			_presentCompleteSemaphores.clear();

			_mainSupportCommandBuffers.destroy();
			_mainRenderCommandBuffers.destroy();

			_depthStencil.destroy();
			_frameBuffers.clear();

			// clear tmp frame data
			{
				std::vector<std::vector<VulkanBuffer*>> tmpBuffers;
				{
					engine::AtomicLock lock(_lockTmpData);
					tmpBuffers = std::move(_tmpBuffers);
					_tmpBuffers.clear();
				}

				for (size_t i = 0, sz = tmpBuffers.size(); i < sz; ++i) {
					for (VulkanBuffer* buffer : tmpBuffers[i]) {
						delete buffer;
					}
				}
				
			}

			if (_width == 0 || _height == 0) { return; }

			_swapChain.resize(_width, _height, _vSync);

			if (_swapchainImagesCount != _swapchainImagesCount) {
				_swapchainImagesCount = _swapchainImagesCount;
			}

			// recreate resources
			_depthStencil = vulkan::VulkanImage(_vulkanDevice, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TYPE_2D, _mainDepthFormat, 1, _width, _height);
			_depthStencil.createImageView();

			_frameBuffers.clear();

			std::vector<VkImageView> attachments(2);
			attachments[1] = _depthStencil.view;

			for (size_t i = 0; i < _swapchainImagesCount; ++i) {
				attachments[0] = _swapChain.images[i].view; // color attachment is the view of the swapchain image
				_frameBuffers.emplace_back(_vulkanDevice, _width, _height, 1, _mainRenderPass, attachments.data(), attachments.size());
			}

			// createCommandBuffers
			_mainRenderCommandBuffers = VulkanCommandBuffersArray(_vulkanDevice->device, _commandPool, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, _swapchainImagesCount);
			_mainSupportCommandBuffers = VulkanCommandBuffersArray(_vulkanDevice->device, _commandPool, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, _swapchainImagesCount);

			_presentCompleteSemaphores.reserve(_swapchainImagesCount);
			for (size_t i = 0; i < _swapchainImagesCount; ++i) {
				_presentCompleteSemaphores.emplace_back(vulkan::VulkanSemaphore(_vulkanDevice->device));
				_mainSupportCommandBuffers.addWaitSemaphore(_presentCompleteSemaphores[i].semaphore, i);
			}

			_mainRenderCommandBuffers.depend(_mainSupportCommandBuffers);

			_waitFences.clear();

			// wait fences to sync command buffer access
			for (uint32_t i = 0; i < _swapchainImagesCount; ++i) {
				_waitFences.emplace_back(vulkan::VulkanFence(_vulkanDevice->device, VK_FENCE_CREATE_SIGNALED_BIT));
			}

			_currentFrame = 0;

			buildDefaultMainRenderCommandBuffer();

			_tmpBuffers.resize(_swapchainImagesCount);
		}
	}

	void VulkanRenderer::beginFrame() {
		// use a fence to wait until the command buffer has finished execution before using it again
		vulkan::waitFenceAndReset(_vulkanDevice->device, _waitFences[_currentFrame].fence);

		if (_mainSupportCommandBuffers[_currentFrame].begin() == VK_SUCCESS) {
			for (auto it = _dinamicUniformBuffers.begin(); it != _dinamicUniformBuffers.end(); ++it) {
				it->second->resetOffset();
			}

			for (auto it = _dinamicStorageBuffers.begin(); it != _dinamicStorageBuffers.end(); ++it) {
				it->second->resetOffset();
			}

			// clear tmp frame data
			if (!_tmpBuffers.empty()) {

				std::vector<VulkanBuffer*> tmpBuffers;
				std::vector<std::pair<VulkanTexture*, VulkanBuffer*>> defferedTextureToGenerate;
				{
					engine::AtomicLock lock(_lockTmpData);
					tmpBuffers = std::move(_tmpBuffers[_currentFrame]);
					defferedTextureToGenerate = std::move(_defferedTextureToGenerate);
					_tmpBuffers[_currentFrame].clear();
					_defferedTextureToGenerate.clear();
				}

				for (VulkanBuffer* buffer : tmpBuffers) { delete buffer; }
				for (auto&& p : defferedTextureToGenerate) { 
					p.first->fillGpuData(p.second, _mainSupportCommandBuffers[_currentFrame]);
					addTmpBuffer(p.second);
				}
			}

		}
	}

	void VulkanRenderer::endFrame() {
		_mainSupportCommandBuffers[_currentFrame].end();

		uint32_t aciquireImageIndex = 0;
		_swapChain.acquireNextImage(_presentCompleteSemaphores[_currentFrame].semaphore, &aciquireImageIndex);

		VkSubmitInfo submitInfos[2];

		submitInfos[0] = _mainSupportCommandBuffers.prepareToSubmit(_currentFrame);
		submitInfos[1] = _mainRenderCommandBuffers.prepareToSubmit(_currentFrame);

		vkQueueSubmit(_mainQueue, 2, &submitInfos[0], _waitFences[_currentFrame].fence);

		// present the current buffer to the swap chain
		// pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
		// this ensures that the image is not presented to the windowing system until all commands have been submitted
		VkResult present = _swapChain.queuePresent(_presentQueue, aciquireImageIndex, _mainRenderCommandBuffers.m_completeSemaphores[_currentFrame].semaphore);
		if (!((present == VK_SUCCESS) || (present == VK_SUBOPTIMAL_KHR))) {
			//VK_CHECK_RESULT(present);
		}

		_currentFrame = (_currentFrame + 1) % _swapchainImagesCount;
	}

	VulkanPipeline* VulkanRenderer::getGraphicsPipeline(
		const VulkanRenderState& renderState,
		const VulkanGpuProgram* program,
		const VkRenderPass renderPass,
		const uint8_t subpass
	) {
		return getGraphicsPipeline(
			renderState.vertexDescription,
			renderState.topology,
			renderState.rasterisationState,
			renderState.blendMode,
			renderState.depthState,
			renderState.stencilState,
			program,
			renderPass,
			subpass
		);
	}

	VulkanPipeline* VulkanRenderer::getGraphicsPipeline(
		const VertexDescription& vertexDescription,
		const VulkanPrimitiveTopology& topology,
		const VulkanRasterizationState& rasterisation,
		const VulkanBlendMode& blendMode,
		const VulkanDepthState& depthState,
		const VulkanStencilState& stencilState,
		const VulkanGpuProgram* program,
		const VkRenderPass renderPass,
		const uint8_t subpass
	) {
		// todo: need synchronisations for cache it
		// get value from cache
		const uint8_t topologyKey = static_cast<uint8_t>(topology.topology) << 0 | static_cast<uint8_t>(topology.enableRestart) << 4;																		// 5 bit
		const uint8_t rasterisationKey = static_cast<uint8_t>(rasterisation.poligonMode)		<< 0 | 
										 static_cast<uint8_t>(rasterisation.cullmode)			<< 2 | 
										 static_cast<uint8_t>(rasterisation.faceOrientation)	<< 4 | 
										 static_cast<uint8_t>(rasterisation.discardEnable)		<< 5;																										// 6 bit
		const uint16_t depthKey = static_cast<uint16_t>(depthState.compareOp) << 0 | static_cast<uint16_t>(depthState.depthTestEnabled) << 3 | static_cast<uint16_t>(depthState.depthWriteEnabled) << 4;	// 5 bit																																							// 16 bit
		const uint16_t programId = program->getId();																																						// 16 bit

		const uint64_t composite_key = static_cast<uint64_t>(topologyKey)		<< 0 |
									   static_cast<uint64_t>(rasterisationKey)	<< 5 |
									   static_cast<uint64_t>(depthKey)			<< 11 |
									   static_cast<uint64_t>(programId)			<< 16 |
									   static_cast<uint64_t>(subpass)			<< 32;																														// 40 bit

		const uint64_t stencil_key = static_cast<uint64_t>(stencilState.enabled)		<< 0 |
									 static_cast<uint64_t>(stencilState.failOp)			<< 1 |
									 static_cast<uint64_t>(stencilState.passOp)			<< 4 |
									 static_cast<uint64_t>(stencilState.depthFailOp)	<< 7 |
									 static_cast<uint64_t>(stencilState.compareOp)		<< 10 |
									 static_cast<uint64_t>(stencilState.compareMask)	<< 13 |
									 static_cast<uint64_t>(stencilState.writeMask)		<< 21 |
									 static_cast<uint64_t>(stencilState.reference)		<< 29;																												// 37 bit

		const uint32_t blendKey = blendMode();

		VkRenderPass currentRenderPass = (renderPass == VK_NULL_HANDLE) ? _mainRenderPass : renderPass;

		GraphicsPipelineCacheKey cacheKey(composite_key, stencil_key, blendKey, currentRenderPass);
		auto it = _graphicsPipelinesCache.find(cacheKey);
		if (it != _graphicsPipelinesCache.end()) {
			return it->second;
		}

		///////////////// create new value and cache it

		// viewport state sets the number of viewports and scissor used in this pipeline
		// this is actually overridden by the dynamic states (see below)
		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		// enable dynamic states
		// most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
		// to be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
		// todo: need setup this?
		std::vector<VkDynamicState> dynamicStateEnables;
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pDynamicStates = dynamicStateEnables.data();
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

		// multi sampling state
		// todo: setup this
		VkPipelineMultisampleStateCreateInfo multisampleState = {};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // no multisampling
		multisampleState.pSampleMask = nullptr;
		//

		////////////// input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.pNext = nullptr;
		inputAssemblyState.flags = 0;
		inputAssemblyState.primitiveRestartEnable = topology.enableRestart;
		inputAssemblyState.topology = static_cast<VkPrimitiveTopology>(topology.topology);

		////////////// depthStencil
		VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		// depth state
		depthStencilState.depthTestEnable = depthState.depthTestEnabled;
		depthStencilState.depthWriteEnable = depthState.depthWriteEnabled;
		depthStencilState.depthCompareOp = depthState.compareOp;

		// stencil state
		depthStencilState.stencilTestEnable = stencilState.enabled;
		depthStencilState.back.failOp = stencilState.failOp;
		depthStencilState.back.passOp = stencilState.passOp;
		depthStencilState.back.compareOp = stencilState.compareOp;

		depthStencilState.back.depthFailOp = stencilState.depthFailOp;
		depthStencilState.back.compareMask = stencilState.compareMask;
		depthStencilState.back.writeMask = stencilState.writeMask;
		depthStencilState.back.reference = stencilState.reference;

		// can set always ??
		depthStencilState.front = depthStencilState.back;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;

		// vertex input binding
		const size_t vertexBindingsSize = vertexDescription.bindings_strides.size();
		std::vector<VkVertexInputBindingDescription> vertexInputBindings(vertexBindingsSize);
		for (size_t i = 0; i < vertexBindingsSize; ++i) {
			vertexInputBindings[i].binding = vertexDescription.bindings_strides[i].first;
			vertexInputBindings[i].stride = vertexDescription.bindings_strides[i].second;
			vertexInputBindings[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		}

		// vertex input state used for pipeline creation
		VkPipelineVertexInputStateCreateInfo vertexInputState = {};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputState.vertexBindingDescriptionCount = vertexBindingsSize;
		vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
		vertexInputState.vertexAttributeDescriptionCount = vertexDescription.attributesCount;
		vertexInputState.pVertexAttributeDescriptions = vertexDescription.attributes;

		///////////////// setup pipeline
		VulkanPipeline* pipeline = new VulkanPipeline();
		_graphicsPipelinesCache[cacheKey] = pipeline;

		// get layout from gpuProgramm
		pipeline->program = program;
		pipeline->renderPass = currentRenderPass;
		pipeline->subpass = subpass;

		VkPipelineRasterizationStateCreateInfo rasterisationInfo = rasterisation.rasterisationInfo();
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentState = blendMode.blendState();

		VkPipelineColorBlendStateCreateInfo colorBlendInfo = {};
		colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendInfo.attachmentCount = static_cast<uint32_t>(blendAttachmentState.size());
		colorBlendInfo.pAttachments = blendAttachmentState.data();
		
		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

		pipelineCreateInfo.pViewportState		= &viewportState;
		pipelineCreateInfo.pDynamicState		= &dynamicState;
		pipelineCreateInfo.pMultisampleState	= &multisampleState;

		pipelineCreateInfo.pInputAssemblyState	= &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState	= &rasterisationInfo;
		pipelineCreateInfo.pColorBlendState		= &colorBlendInfo;
		pipelineCreateInfo.pDepthStencilState	= &depthStencilState;

		pipelineCreateInfo.pVertexInputState	= &vertexInputState;

		// set pipeline shader stage info
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(program->m_shaderStages.size());
		pipelineCreateInfo.pStages = program->m_shaderStages.data();

		// the layout used for this pipeline (can be shared among multiple pipelines using the same layout)
		pipelineCreateInfo.layout = program->getPipeLineLayout();
		// renderpass this pipeline is attached to
		pipelineCreateInfo.renderPass = pipeline->renderPass;
		pipelineCreateInfo.subpass = subpass;

		// create rendering pipeline using the specified states
		vkCreateGraphicsPipelines(_vulkanDevice->device, _pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline->pipeline);

		return pipeline;
	}

	VulkanDynamicBuffer* VulkanRenderer::getDynamicUniformBufferForSize(const uint32_t size) {
		// todo: need synchronisation
		if (size == 0) return nullptr;

		auto it = _dinamicUniformBuffers.find(size);
		if (it != _dinamicUniformBuffers.end()) {
			return it->second;
		}

		constexpr uint32_t dynamicUniformBuffersCount = 1024; // todo: configure this value
		VulkanDynamicBuffer* newDynamicBuffer = new VulkanDynamicBuffer(size, _swapchainImagesCount, dynamicUniformBuffersCount);

		const uint32_t bufferSize = static_cast<uint32_t>(dynamicUniformBuffersCount * size);
		for (size_t i = 0; i < _swapchainImagesCount; ++i) {
			_vulkanDevice->createBuffer(
				VK_SHARING_MODE_EXCLUSIVE,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&newDynamicBuffer->buffers[i],
				bufferSize
			);
		}

		newDynamicBuffer->map();

		_dinamicUniformBuffers.emplace(size, newDynamicBuffer);
		return newDynamicBuffer;
	}

	uint32_t VulkanRenderer::updateDynamicUniformBufferData(VulkanDynamicBuffer* dynamicBuffer, const void* data, const uint32_t offset, const uint32_t size, const bool allBuffers, const uint32_t knownOffset) const {
		if (dynamicBuffer == nullptr) return 0;
		// todo: think about max count of mapped dynamic buffers
		const uint32_t bufferOffset = dynamicBuffer->alignedSize * (knownOffset == 0xffffffff ? dynamicBuffer->getCurrentOffset() : knownOffset);
		//dynamicBuffer->map();
		if (allBuffers) {
			for (size_t i = 0, sz = dynamicBuffer->memory.size(); i < sz; ++i) {
				memcpy(reinterpret_cast<void*>(reinterpret_cast<size_t>(dynamicBuffer->memory[i]) + bufferOffset + offset), data, size);
			}
		} else {
			memcpy(reinterpret_cast<void*>(reinterpret_cast<size_t>(dynamicBuffer->memory[_currentFrame]) + bufferOffset + offset), data, size);
		}
		//dynamicBuffer->unmap();
		return bufferOffset;
	}

	void VulkanRenderer::bindDynamicUniformBufferToDescriptorSet(const VulkanDescriptorSet* descriptorSet, VulkanDynamicBuffer* dynamicBuffer, const uint32_t binding) const {
		bindBufferToDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, dynamicBuffer->buffers.data(), binding, dynamicBuffer->alignedSize, 0);
	}

	void VulkanRenderer::waitWorkComplete() const {
		if (_instance != VK_NULL_HANDLE) {
			if (_vulkanDevice != nullptr) {

				vkQueueWaitIdle(_mainQueue);
				vkQueueWaitIdle(_presentQueue);

				_vulkanDevice->waitIdle();
			}
		}
	}

	void VulkanRenderer::destroy() {
		if (_instance != VK_NULL_HANDLE) {
			if (_vulkanDevice != nullptr) {

				waitWorkComplete();

				_mainRenderCommandBuffers.destroy();
				_mainSupportCommandBuffers.destroy();

				_waitFences.clear();

				_vulkanDevice->destroyRenderPass(_mainRenderPass);
				_frameBuffers.clear();

				delete _emptyTexture;

				for (auto it = _dinamicUniformBuffers.begin(); it != _dinamicUniformBuffers.end(); ++it) {
					it->second->unmap();
					delete it->second;
				}
				_dinamicUniformBuffers.clear();

				for (auto it = _dinamicStorageBuffers.begin(); it != _dinamicStorageBuffers.end(); ++it) {
					it->second->unmap();
					delete it->second;
				}
				_dinamicStorageBuffers.clear();

				for (auto it = _graphicsPipelinesCache.begin(); it != _graphicsPipelinesCache.end(); ++it) {
					vkDestroyPipeline(_vulkanDevice->device, it->second->pipeline, nullptr);
					delete it->second;
				}
				_graphicsPipelinesCache.clear();

				for (auto&& cdl : _descriptorLayoutsCache) {
					vkDestroyPipelineLayout(_vulkanDevice->device, cdl->layout.pipelineLayout, nullptr);
					uint16_t n = 0;
					for (auto dsl : cdl->layout.descriptorSetLayouts) {
						if (cdl->descriptorSetLayoutsHandled & (1 << n)) {
							vkDestroyDescriptorSetLayout(_vulkanDevice->device, dsl, nullptr);
						}
						++n;
					}
					delete cdl; 
				}
				_descriptorLayoutsCache.clear();

				_swapChain.clear();

				vkDestroyPipelineCache(_vulkanDevice->device, _pipelineCache, nullptr);
				vkDestroyDescriptorPool(_vulkanDevice->device, _globalDescriptorPool, nullptr);

				_depthStencil.destroy();

				for (auto&& semaphore : _presentCompleteSemaphores) {
					semaphore.destroy();
				}
				_presentCompleteSemaphores.clear();


				for (auto it = _samplers.begin(); it != _samplers.end(); ++it) {
					vkDestroySampler(_vulkanDevice->device, it->second, nullptr);
				}
				_samplers.clear();

				// clear tmp frame data
				std::vector<std::vector<VulkanBuffer*>> tmpBuffers;
				std::vector<std::pair<VulkanTexture*, VulkanBuffer*>> defferedTextureToGenerate;
				{
					engine::AtomicLock lock(_lockTmpData);
					tmpBuffers = std::move(_tmpBuffers);
					defferedTextureToGenerate = std::move(_defferedTextureToGenerate);
					_tmpBuffers.clear();
					_defferedTextureToGenerate.clear();
				}

				for (size_t i = 0, sz = tmpBuffers.size(); i < sz; ++i) {
					for (VulkanBuffer* buffer : tmpBuffers[i]) {
						delete buffer;
					}
				}

				for (auto&& p : defferedTextureToGenerate) {
					delete p.second;
				}

				tmpBuffers.clear();
				defferedTextureToGenerate.clear();

				delete _vulkanDevice;
				_vulkanDevice = nullptr;
			}

			vkDestroyInstance(_instance, nullptr);
			_instance = VK_NULL_HANDLE;
		}
	}
}