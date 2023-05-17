#include "vkRenderer.h"
#include "vkHelper.h"

#include "../RenderSurfaceInitializer.h"
#include "vkTexture.h"

#include "vkDebugMarker.h"

#include "../../Engine/Core/Configs.h"
#include "../../Engine/Log/Log.h"

#include <unordered_map>
#include <unordered_set>
#include <cassert>

#define EXECUTE_PROC_OR_NULLIFY_AND_RETURN(x, proc, ...) if (!x->proc(__VA_ARGS__)) { delete x; return nullptr; }

namespace vulkan {

// ---------- initialisation ----------
	VulkanRenderer::VulkanRenderer() : _width(1024), _height(768), _vSync(false) { }

	VulkanRenderer::~VulkanRenderer() {
		destroy();
	}

	bool VulkanRenderer::createInstance(
            const engine::Version apiVersion,
            const engine::Version engineVersion,
            const engine::Version applicationVersion,
            const std::vector<VkExtensionProperties>& desiredInstanceExtensions,
            const std::vector<VkLayerProperties>& desiredLayers) {

		VkApplicationInfo vk_applicationInfo;
		vk_applicationInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_APPLICATION_INFO;
		vk_applicationInfo.pNext = nullptr;
		vk_applicationInfo.pApplicationName     = "j4f_application";
        vk_applicationInfo.pEngineName          = "j4f_engine";
		vk_applicationInfo.applicationVersion   = applicationVersion();
		vk_applicationInfo.engineVersion        = engineVersion();
		vk_applicationInfo.apiVersion           = VK_MAKE_VERSION(apiVersion->major, apiVersion->minor, apiVersion->patch);

		std::vector<VkExtensionProperties> supportedExtensions;
		getVulkanInstanceSupportedExtensions(supportedExtensions);

		std::vector<const char*> instanceExtensions = { "VK_KHR_surface" };

		std::vector<VkLayerProperties> supportedLayers;
		getVulkanInstanceSupportedLayers(supportedLayers);

		std::vector<const char*> instanceLayers;

#ifdef _DEBUG
#ifdef GPU_VALIDATION_ENABLED
		constexpr bool validationEnable = true;
#else
        constexpr bool validationEnable = false;
#endif // GPU_VALIDATION_ENABLED
#else
        constexpr bool validationEnable = false;
#endif

		auto addInstanceExtension = [&instanceExtensions, &supportedExtensions](const char* extName) -> bool {
			if (std::find_if(supportedExtensions.begin(), supportedExtensions.end(), [extName](const VkExtensionProperties& ext) { return strcmp(ext.extensionName, extName) == 0; }) != supportedExtensions.end()) {
				instanceExtensions.push_back(extName);
				return true;
			}
			return false;
		};

		auto addInstanceLayer = [&instanceLayers, &supportedLayers](const char* layerName) {
			if (std::find_if(supportedLayers.begin(), supportedLayers.end(), [layerName](const VkLayerProperties& p) { return strcmp(p.layerName, layerName) == 0; }) != supportedLayers.end()) {
				instanceLayers.push_back(layerName);
			}
		};

		// https://vulkan.lunarg.com/doc/view/1.3.211.0/linux/LoaderDriverInterface.html
		constexpr std::array<const char*, 6> instanceSurfaceExtensions = {
																"VK_KHR_win32_surface", "VK_KHR_xcb_surface", "VK_KHR_xlib_surface", 
																"VK_KHR_wayland_surface", "VK_MVK_macos_surface", "VK_QNX_screen_surface"
																};
		for (const char* ext : instanceSurfaceExtensions) {
			if (addInstanceExtension(ext)) {
				break;
			}
		}

		if constexpr (validationEnable) {
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
		vk_instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
		vk_instanceCreateInfo.ppEnabledLayerNames = instanceLayers.data();
		vk_instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
		vk_instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

		const VkResult result = vkCreateInstance(&vk_instanceCreateInfo, nullptr, &_instance);
		return result == VkResult::VK_SUCCESS;
	}

	bool VulkanRenderer::createDevice(const VkPhysicalDeviceFeatures features, std::vector<const char*>& extensions, const VkPhysicalDeviceType deviceType) {
		uint32_t vk_physicalDevicesCount;
		if (vkEnumeratePhysicalDevices(_instance, &vk_physicalDevicesCount, nullptr) != VkResult::VK_SUCCESS) { return false; }

		std::vector<VkPhysicalDevice> vk_physicalDevices(vk_physicalDevicesCount);
		if (vkEnumeratePhysicalDevices(_instance, &vk_physicalDevicesCount, &vk_physicalDevices[0]) != VkResult::VK_SUCCESS) { return false; }

		if (vk_physicalDevices.empty()) { return false; }

//		extensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME); // for negative viewPort height

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

        constexpr static std::array<uint8_t, 5> deviceTypePrioritets = {
                4, // VK_PHYSICAL_DEVICE_TYPE_OTHER = 0
                1, // VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU = 1
                0, // VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU = 2
                3, // VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU = 3
                2  // VK_PHYSICAL_DEVICE_TYPE_CPU = 4
        };

		if (vk_physicalDevicesCount > 1) {
			for (VkPhysicalDevice gpu : vk_physicalDevices) {
				// check support features + extensions 
				// get desired deviceType if exist and it ok
				auto* condidate = new vulkan::VulkanDevice(gpu);
				if (checkGPU(condidate)) {
					if (condidate->gpuProperties.deviceType == deviceType) {
                        if (_vulkanDevice) {
                            delete _vulkanDevice;
                        }

                        _vulkanDevice = condidate;
						break;
					}

                    if (_vulkanDevice == nullptr ||
                        deviceTypePrioritets[_vulkanDevice->gpuProperties.deviceType] > deviceTypePrioritets[condidate->gpuProperties.deviceType]) {
                        if (_vulkanDevice) {
                            delete _vulkanDevice;
                        }
                        _vulkanDevice = condidate;
                    } else {
                        delete condidate;
                    }

				} else {
					delete condidate;
				}
			}
		} else {
			auto* condidate = new vulkan::VulkanDevice(vk_physicalDevices[0]);
			if (checkGPU(condidate)) {
				_vulkanDevice = condidate;
			} else {
				delete condidate;
			}
		}

		if (_vulkanDevice == nullptr) { // error no device with supported features + extensions
			return false;
		}

		GPU_DEBUG_MARKERS_INIT(_vulkanDevice, extensions);

		const VkResult res = _vulkanDevice->createDevice(features, extensions, _deviceCreatepNextChain);

		GPU_DEBUG_MARKERS_SETUP(_vulkanDevice);

		return res == VK_SUCCESS;
	}

	void VulkanRenderer::createSwapChain(const engine::IRenderSurfaceInitializer* initializer, const bool useVsync) {
		_vSync = useVsync;
		_swapChain.connect(_instance, _vulkanDevice, initializer);
		_swapChain.resize(_width, _height, useVsync);
	}

	void VulkanRenderer::init(const engine::GraphicConfig& cfg) {
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
			_presentCompleteSemaphores.emplace_back(_vulkanDevice->device);
			_mainSupportCommandBuffers.addWaitSemaphore(_presentCompleteSemaphores[i].semaphore, i);
		}

		_mainRenderCommandBuffers.depend(_mainSupportCommandBuffers);

		// wait fences to sync command buffer access
		for (uint32_t i = 0; i < _swapchainImagesCount; ++i) {
			_waitFences.emplace_back(_vulkanDevice->device, VK_FENCE_CREATE_SIGNALED_BIT);
		}

		// find a suitable depth format
		_mainDepthFormat = _vulkanDevice->getSupportedDepthFormat(_mainDepthFormatBits);

		// setupDepthStencil
		_depthStencil = vulkan::VulkanImage(_vulkanDevice, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TYPE_2D, _mainDepthFormat, 1, _width, _height);
		_depthStencil.createImageView();

		// createPipelineCache
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		vkCreatePipelineCache(_vulkanDevice->device, &pipelineCacheCreateInfo, nullptr, &_pipelineCache);

		// setupRenderPass
		setupRenderPass({}, {}, cfg.can_continue_main_render_pass); // todo: VkSubpassDependency parametrisation

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

        _buffersToDelete.resize(_swapchainImagesCount);
        _texturesToDelete.resize(_swapchainImagesCount);

		createEmtyTexture();
	}

	void VulkanRenderer::createEmtyTexture() {
#ifdef _DEBUG
		_emptyTexture = new VulkanTexture(this, 2, 2, 1);
		_emptyTextureArray = new VulkanTexture(this, 2, 2, 1);
		const unsigned char emptyImg[16] = { 
			100, 100, 100, 255, 160, 160, 160, 255,
			160, 160, 160, 255, 100, 100, 100, 255
	};
#else
		//_emptyTexture = new VulkanTexture(this, 1, 1, 1);
		//_emptyTextureArray = new VulkanTexture(this, 1, 1, 1);
		//const unsigned char emptyImg[4] = { 200, 200, 200, 255 };

		_emptyTexture = new VulkanTexture(this, 2, 2, 1);
		_emptyTextureArray = new VulkanTexture(this, 2, 2, 1);
		const unsigned char emptyImg[16] = { 
			100, 100, 100, 255, 160, 160, 160, 255,
			160, 160, 160, 255, 100, 100, 100, 255
		};

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

		_emptyTextureArray->setSampler(getSampler(
			VK_FILTER_NEAREST,
			VK_FILTER_NEAREST,
			VK_SAMPLER_MIPMAP_MODE_NEAREST,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK
		));

		const void* emtyImageAddr = &emptyImg[0];

		_emptyTexture->create(&emtyImageAddr, 1, VK_FORMAT_R8G8B8A8_UNORM, 32, false, false, VK_IMAGE_VIEW_TYPE_2D);
		_emptyTexture->createSingleDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0);

		_emptyTextureArray->create(&emtyImageAddr, 1, VK_FORMAT_R8G8B8A8_UNORM, 32, false, false, VK_IMAGE_VIEW_TYPE_2D_ARRAY);
		_emptyTextureArray->createSingleDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0);
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
		// addressModes
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

			// Update dynamic depth bias state
			commandBuffer.cmdSetDepthBias(0.0f, 0.0f, 0.0f);

			///////////////// finish cmd buffer
			commandBuffer.cmdEndRenderPass();

			// Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to
			// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system
			commandBuffer.end();
		}
	}

	void VulkanRenderer::setupRenderPass(const std::vector<VkAttachmentDescription>& configuredAttachments, const std::vector<VkSubpassDependency>& configuredDependencies, const bool canContinueMainRenderPass) {

		auto createDefaultAttachments = [this, canContinueMainRenderPass]()->std::vector<VkAttachmentDescription> {
			std::vector<VkAttachmentDescription> attachments(2);
			// color attachment
			attachments[0] = vulkan::createAttachmentDescription(_swapChain.colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
			// depth attachment
			if (canContinueMainRenderPass) {
				attachments[1] = vulkan::createAttachmentDescription(_mainDepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
			} else {
				attachments[1] = vulkan::createAttachmentDescription(_mainDepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);
			}
			
			return attachments;
		};

		auto createDefaultContinueAttachments = [this]()->std::vector<VkAttachmentDescription> {
			std::vector<VkAttachmentDescription> attachments(2);
			// color attachment
			attachments[0] = vulkan::createAttachmentDescription(_swapChain.colorFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);
			// depth attachment
			// если воспользуемся продолжением отрисовки - то видимо нужно хранить результат для depthStencil
			attachments[1] = vulkan::createAttachmentDescription(_mainDepthFormat, 
																VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
																VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
																VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, 
																VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE
																);
			return attachments;
		};

		const std::vector<VkAttachmentDescription>& attachments = configuredAttachments.empty() ? createDefaultAttachments () : configuredAttachments;
		const std::vector<VkAttachmentDescription>& continueAttachments = createDefaultContinueAttachments();

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
			_mainContinueRenderPass = createRenderPass(_vulkanDevice, VK_PIPELINE_BIND_POINT_GRAPHICS, dependencies, continueAttachments);
		} else {
			_mainRenderPass = createRenderPass(_vulkanDevice, VK_PIPELINE_BIND_POINT_GRAPHICS, configuredDependencies, attachments);
			_mainContinueRenderPass = createRenderPass(_vulkanDevice, VK_PIPELINE_BIND_POINT_GRAPHICS, configuredDependencies, continueAttachments);
		}
	}

	VkResult VulkanRenderer::setupDescriptorPool(const std::vector<VkDescriptorPoolSize>& descriptorPoolCfg) {
		//https://habr.com/ru/post/584554/

		// create the global descriptor pool
		// all descriptors used in this example are allocated from this pool
		VkDescriptorPoolCreateInfo descriptorPoolInfo;
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.pNext = nullptr;
		descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		
		VkResult result;
		VkDescriptorPool pool;

		if (descriptorPoolCfg.empty()) { // default variant
			// we need to tell the API the number of max. requested descriptors per type
			VkDescriptorPoolSize typeCounts[5];
			typeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			typeCounts[0].descriptorCount = 64;

			typeCounts[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			typeCounts[1].descriptorCount = 512;

			typeCounts[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			typeCounts[2].descriptorCount = 16;

			typeCounts[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			typeCounts[3].descriptorCount = 64;

			typeCounts[4].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			typeCounts[4].descriptorCount = 1024;

			// set the max. number of descriptor sets that can be requested from this pool (requesting beyond this limit will result in an error)
			descriptorPoolInfo.maxSets =
				typeCounts[0].descriptorCount +
				typeCounts[1].descriptorCount +
				typeCounts[2].descriptorCount +
				typeCounts[3].descriptorCount +
				typeCounts[4].descriptorCount;

			descriptorPoolInfo.poolSizeCount = 5;
			descriptorPoolInfo.pPoolSizes = &typeCounts[0];
			result = vkCreateDescriptorPool(_vulkanDevice->device, &descriptorPoolInfo, nullptr, &pool);
		} else {
			if (_descriptorPoolCustomConfig.empty()) { _descriptorPoolCustomConfig = descriptorPoolCfg; }

			descriptorPoolInfo.maxSets = 0;
			for (auto&& c : descriptorPoolCfg) {
				descriptorPoolInfo.maxSets += c.descriptorCount;
			}

			descriptorPoolInfo.poolSizeCount = descriptorPoolCfg.size();
			descriptorPoolInfo.pPoolSizes = &descriptorPoolCfg[0];
			result = vkCreateDescriptorPool(_vulkanDevice->device, &descriptorPoolInfo, nullptr, &pool);
		}

		if (result == VK_SUCCESS) {
			_currentDescriptorPool = _globalDescriptorPools.size();
			_globalDescriptorPools.push_back(pool);

			//LOG_TAG_LEVEL(engine::LogLevel::L_CUSTOM, GRAPHICS, "VulkanRenderer allocate descriptorPool(maxSets = {}), descriptorPools size = {}", descriptorPoolInfo.maxSets, _globalDescriptorPools.size());
            LOG_TAG_LEVEL(engine::LogLevel::L_CUSTOM, GRAPHICS, "VulkanRenderer allocate descriptorPool(maxSets = %d), descriptorPools size = %d", descriptorPoolInfo.maxSets, _globalDescriptorPools.size());
		} else {
			//LOG_TAG_LEVEL(engine::LogLevel::L_ERROR, GRAPHICS, "VulkanRenderer allocate descriptorPool(maxSets = {}), descriptorPools size = {} error: {}", descriptorPoolInfo.maxSets, _globalDescriptorPools.size(), result);
            LOG_TAG_LEVEL(engine::LogLevel::L_ERROR, GRAPHICS, "VulkanRenderer allocate descriptorPool(maxSets = %d), descriptorPools size = %d error: %s", descriptorPoolInfo.maxSets, _globalDescriptorPools.size(), string_VkResult(result));
		}

		return result;
	}

	PipelineDescriptorLayout& VulkanRenderer::getDescriptorLayout(const std::vector<std::vector<VkDescriptorSetLayoutBinding*>>& setLayoutBindings, const std::vector<VkPushConstantRange*>& pushConstantsRanges) {
		// todo: need multithreading synchronisation
		const size_t constantsCount = pushConstantsRanges.size();
		std::vector<uint64_t> constantsCharacterVec(constantsCount);

		for (size_t i = 0; i < constantsCount; ++i) {
			const auto stageFlags = static_cast<uint8_t>(pushConstantsRanges[i]->stageFlags);
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
				const auto descriptorType = static_cast<uint8_t>(bindings[i]->descriptorType);
				const uint8_t descriptorCount = bindings[i]->descriptorCount;
				const auto stageFlags = static_cast<uint8_t>(bindings[i]->stageFlags);
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
		for (size_t j = 0; j < constantsCount; ++j) {
			pushConstants[j] = *(pushConstantsRanges[j]);
		}

		VkPipelineLayout pipelineLayout;
		if (isEmptyLayoutBindings) {
			_vulkanDevice->createPipelineLayout(&pipelineLayout, 0, VK_NULL_HANDLE, constantsCount, &pushConstants[0]);
		} else {
			_vulkanDevice->createPipelineLayout(&pipelineLayout, descriptorSetLayouts.size(), &descriptorSetLayouts[0], constantsCount, &pushConstants[0]);
		}

		auto* layout = new CachedDescriptorLayouts(std::move(bindingsCharacterVec), std::move(constantsCharacterVec), { pipelineLayout, descriptorSetLayouts }, descriptorSetLayoutsHandled);
		_descriptorLayoutsCache.push_back(layout);
		return layout->layout;
	}

	VkResult VulkanRenderer::allocateDescriptorSetsFromGlobalPool(VkDescriptorSetAllocateInfo& allocInfo, VkDescriptorSet* set) {
		VkResult result = vkAllocateDescriptorSets(_vulkanDevice->device, &allocInfo, set);

		switch (result) {
			case VK_ERROR_INITIALIZATION_FAILED:
			case VK_ERROR_FRAGMENTED_POOL:
			case VK_ERROR_OUT_OF_POOL_MEMORY:
			{
				bool allocateFromExistingPool = false;
				for (uint32_t i = 0, sz = _globalDescriptorPools.size(); i < sz; ++i) {
					if (i == _currentDescriptorPool) { continue; }

					allocInfo.descriptorPool = _globalDescriptorPools[i];
					result = vkAllocateDescriptorSets(_vulkanDevice->device, &allocInfo, set); // try allocate from existing pools
					if (result == VK_SUCCESS) {
						_currentDescriptorPool = i;
						allocateFromExistingPool = true;
						break;
					}
				}

				if (!allocateFromExistingPool) {
					if (setupDescriptorPool(_descriptorPoolCustomConfig) == VK_SUCCESS) { // allocate new pool
						allocInfo.descriptorPool = _globalDescriptorPools[_currentDescriptorPool];
						result = vkAllocateDescriptorSets(_vulkanDevice->device, &allocInfo, set);
					}
				}
			}
				break;
			default:
				break;
		}

		if (result != VK_SUCCESS) {
            LOG_TAG_LEVEL(engine::LogLevel::L_ERROR, GRAPHICS, "allocate descriptor set error: %s", string_VkResult(result));
		}

		return result;
	}

	VulkanDescriptorSet* VulkanRenderer::allocateDescriptorSetFromGlobalPool(const VkDescriptorSetLayout descriptorSetLayout, const uint32_t count) {
		const uint32_t setsCount = count == 0 ? _swapchainImagesCount : count;
	
		auto* descriptorSet = new VulkanDescriptorSet(setsCount);
		std::vector<VkDescriptorSetLayout> layouts(setsCount, descriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.descriptorPool = _globalDescriptorPools[_currentDescriptorPool];
		allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
		allocInfo.pSetLayouts = &layouts[0];

		VkResult result = allocateDescriptorSetsFromGlobalPool(allocInfo, &descriptorSet->set[0]);

		if (result != VK_SUCCESS) {
			return nullptr;
		}

		descriptorSet->parentPool = _globalDescriptorPools[_currentDescriptorPool];
		return descriptorSet;
	}

	std::pair<VkDescriptorSet, uint32_t> VulkanRenderer::allocateSingleDescriptorSetFromGlobalPool(const VkDescriptorSetLayout descriptorSetLayout) {
		VkDescriptorSet descriptorSet;

		VkDescriptorSetAllocateInfo allocInfo;
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.descriptorPool = _globalDescriptorPools[_currentDescriptorPool];
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		allocateDescriptorSetsFromGlobalPool(allocInfo, &descriptorSet);
		return { descriptorSet, _currentDescriptorPool };
	}

	void VulkanRenderer::bindBufferToDescriptorSet(
		const VulkanDescriptorSet* descriptorSet,
		const VkDescriptorType type, 
		const VulkanBuffer* buffer,
		const uint32_t binding,
		const uint32_t alignedSize,
		const uint32_t offset
	) const {
		const auto setsCount = static_cast<uint32_t>(descriptorSet->set.size());
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
		const auto setsCount = static_cast<uint32_t>(descriptorSet->set.size());
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

			for (auto&& fence : _waitFences) {
				fence.destroy();
			}
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
				std::vector<std::vector<VulkanBuffer*>> buffersToDelete;
                std::vector<std::vector<VulkanTexture*>> texturesToDelete;
				{
					engine::AtomicLock lock(_lockTmpData);
                    buffersToDelete = std::move(_buffersToDelete);
                    texturesToDelete = std::move(_texturesToDelete);
                    _buffersToDelete.clear();
                    _texturesToDelete.clear();
				}

                for (auto&& buffers : buffersToDelete) {
                    for (auto* buffer : buffers) {
                        delete buffer;
                    }
                }

                for (auto&& textures : texturesToDelete) {
                    for (auto* texture : textures) {
                        delete texture;
                    }
                }
			}

			if (_width == 0 || _height == 0) { 
				//assert(false);
				return;
			}

			_swapChain.resize(_width, _height, _vSync);

			if (_swapchainImagesCount != _swapChain.imageCount) {
				_swapchainImagesCount = _swapChain.imageCount;
			}

			// recreate resources
			_depthStencil = vulkan::VulkanImage(_vulkanDevice, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TYPE_2D, _mainDepthFormat, 1, _width, _height);
			_depthStencil.createImageView();

			_frameBuffers.clear();

			std::vector<VkImageView> attachments(2);
			attachments[1] = _depthStencil.view;

			for (size_t i = 0; i < _swapchainImagesCount; ++i) {
				attachments[0] = _swapChain.images[i].view; // color attachment is the view of the swapchain image
                VkResult result;
				_frameBuffers.emplace_back(_vulkanDevice, _width, _height, 1,
                                           _mainRenderPass, attachments.data(),
                                           attachments.size(), nullptr,
                                           &result);

                if (result != VK_SUCCESS) {
                    LOG_TAG_LEVEL(engine::LogLevel::L_ERROR, GRAPHICS, "VulkanRenderer: VulkanFrameBuffer create result = %s", string_VkResult(result));
                }
			}

			// createCommandBuffers
			_mainRenderCommandBuffers = VulkanCommandBuffersArray(_vulkanDevice->device, _commandPool, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, _swapchainImagesCount);
			_mainSupportCommandBuffers = VulkanCommandBuffersArray(_vulkanDevice->device, _commandPool, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, _swapchainImagesCount);

			_presentCompleteSemaphores.reserve(_swapchainImagesCount);
			for (size_t i = 0; i < _swapchainImagesCount; ++i) {
				auto&& semaphore = _presentCompleteSemaphores.emplace_back(_vulkanDevice->device);
				_mainSupportCommandBuffers.addWaitSemaphore(semaphore.semaphore, i);
			}

			_mainRenderCommandBuffers.depend(_mainSupportCommandBuffers);

			_waitFences.reserve(_swapchainImagesCount);
			// wait fences to sync command buffer access
			for (uint32_t i = 0; i < _swapchainImagesCount; ++i) {
				_waitFences.emplace_back(_vulkanDevice->device, VK_FENCE_CREATE_SIGNALED_BIT);
			}

			buildDefaultMainRenderCommandBuffer();

            _buffersToDelete.resize(_swapchainImagesCount);
            _texturesToDelete.resize(_swapchainImagesCount);

			_currentFrame = 0;
		}
	}

	void VulkanRenderer::beginFrame() {
		// use a fence to wait until the command buffer has finished execution before using it again
		if (auto result = vulkan::waitFenceAndReset(_vulkanDevice->device, _waitFences[_currentFrame].fence); result != VK_SUCCESS) {
            LOG_TAG_LEVEL(engine::LogLevel::L_ERROR, GRAPHICS, "VulkanRenderer: waitFenceAndReset result = %s", string_VkResult(result));
            return;
        }

		if (_mainSupportCommandBuffers[_currentFrame].begin() == VK_SUCCESS) {
            for (auto&& [size, buffer] : _dinamicGPUBuffers) {
                buffer->resetOffset();
            }

			// clear tmp frame data
			if (!_buffersToDelete.empty() || !_texturesToDelete.empty()) {
				std::vector<VulkanBuffer*> buffersToDelete;
                std::vector<VulkanTexture*> texturesToDelete;
				std::vector<std::tuple<VulkanTexture*, VulkanBuffer*, uint32_t, uint32_t>> defferedTextureToGenerate;

				{
					engine::AtomicLock lock(_lockTmpData);
                    buffersToDelete = std::move(_buffersToDelete[_currentFrame]);
                    texturesToDelete = std::move(_texturesToDelete[_currentFrame]);
					defferedTextureToGenerate = std::move(_defferedTextureToGenerate);
                    _buffersToDelete[_currentFrame].clear();
                    _texturesToDelete[_currentFrame].clear();
					_defferedTextureToGenerate.clear();
				}

				for (auto* buffer : buffersToDelete) {
                    delete buffer;
                }

                for (auto* texture : texturesToDelete) {
                    delete texture;
                }

				for (auto&& [texture, buffer, baseLayer, layersCount] : defferedTextureToGenerate) {
                    texture->fillGpuData(buffer, _mainSupportCommandBuffers[_currentFrame], baseLayer, layersCount);
					markToDelete(buffer);
				}
			}
		}
	}

	void VulkanRenderer::endFrame() {
		_mainSupportCommandBuffers[_currentFrame].end();

		uint32_t aciquireImageIndex = 0;
		const VkResult result = _swapChain.acquireNextImage(_presentCompleteSemaphores[_currentFrame].semaphore, &aciquireImageIndex);
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) { // acquire failed - skip this frame to present
			const VkResult fenceStatus = vkGetFenceStatus(_vulkanDevice->device, _waitFences[_currentFrame].fence);
			if (fenceStatus != VK_SUCCESS) {
				_waitFences[_currentFrame].destroy();
				_waitFences[_currentFrame] = vulkan::VulkanFence(_vulkanDevice->device, VK_FENCE_CREATE_SIGNALED_BIT);
			}
			_currentFrame = (_currentFrame + 1) % _swapchainImagesCount;
            LOG_TAG_LEVEL(engine::LogLevel::L_ERROR, GRAPHICS, "VulkanRenderer: swapChain acquireNextImage result = %s", string_VkResult(result));
			return;
		}

		VkSubmitInfo submitInfos[2];

		submitInfos[0] = _mainSupportCommandBuffers.prepareToSubmit(_currentFrame);
		submitInfos[1] = _mainRenderCommandBuffers.prepareToSubmit(_currentFrame);

        const VkResult queueResult = vkQueueSubmit(_mainQueue, 2, &submitInfos[0], _waitFences[_currentFrame].fence);

        switch (queueResult) {
            case VK_ERROR_DEVICE_LOST:
                LOG_TAG_LEVEL(engine::LogLevel::L_ERROR, GRAPHICS, "VulkanRenderer: device lost!!!!");
                return;
            default:
                LOG_TAG_LEVEL(engine::LogLevel::L_ERROR, GRAPHICS, "VulkanRenderer: vkQueueSubmit result = %s", string_VkResult(queueResult));
            case VK_SUCCESS:
            {
                // present the current buffer to the swap chain
                // pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
                // this ensures that the image is not presented to the windowing system until all commands have been submitted
                const VkResult presentResult = _swapChain.queuePresent(_presentQueue, aciquireImageIndex,
                                                                       _mainRenderCommandBuffers.m_completeSemaphores[_currentFrame].semaphore);
                if (!((presentResult == VK_SUCCESS) || (presentResult == VK_SUBOPTIMAL_KHR))) {
                    LOG_TAG_LEVEL(engine::LogLevel::L_ERROR, GRAPHICS, "VulkanRenderer: _swapChain queuePresent = %s", string_VkResult(presentResult));
                }
            }
                break;
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
			renderState.rasterizationState,
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
		const VulkanRasterizationState& rasterization,
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
		const uint8_t rasterizationKey = static_cast<uint8_t>(rasterization.poligonMode)		<< 0 |
										 static_cast<uint8_t>(rasterization.cullMode)			<< 2 |
										 static_cast<uint8_t>(rasterization.faceOrientation)	<< 4 |
										 static_cast<uint8_t>(rasterization.discardEnable)		<< 5;																										// 6 bit
		const uint16_t depthKey = static_cast<uint16_t>(depthState.compareOp) << 0 | static_cast<uint16_t>(depthState.depthTestEnabled) << 3 | static_cast<uint16_t>(depthState.depthWriteEnabled) << 4;	// 5 bit																																							// 16 bit
		const uint16_t programId = program->getId();																																						// 16 bit

		const uint64_t composite_key = static_cast<uint64_t>(topologyKey)		<< 0 |
									   static_cast<uint64_t>(rasterizationKey)	<< 5 |
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

		const VkRenderPass currentRenderPass = (renderPass == VK_NULL_HANDLE) ? _mainRenderPass : renderPass;

		const GraphicsPipelineCacheKey cacheKey(composite_key, stencil_key, blendKey, currentRenderPass);
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
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);

        if (_vulkanDevice->enabledFeatures.wideLines != 0) {
            dynamicStateEnables.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
        }

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

		const VkPipelineRasterizationStateCreateInfo rasterizationInfo = rasterization.rasterizationInfo();
		const std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentState = blendMode.blendState();

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
		pipelineCreateInfo.pRasterizationState	= &rasterizationInfo;
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

		GPU_DEBUG_MARKER_SET_OBJECT_NAME(pipeline->pipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, engine::fmt_string("j4f graphics pipeline (%d)", _graphicsPipelinesCache.size() - 1));

		return pipeline;
	}

	VulkanDynamicBuffer* VulkanRenderer::getDynamicGPUBufferForSize(const uint32_t size, const VkBufferUsageFlags usageFlags, const uint32_t maxCount) {
		// todo: need synchronisation
		if (size == 0) return nullptr;

		auto it = _dinamicGPUBuffers.find(size);
		if (it != _dinamicGPUBuffers.end()) {
			return it->second;
		}

		auto* newDynamicBuffer = new VulkanDynamicBuffer(size, _swapchainImagesCount, maxCount);

		const auto bufferSize = static_cast<uint32_t>(maxCount * size);
		for (size_t i = 0; i < _swapchainImagesCount; ++i) {
			_vulkanDevice->createBuffer(
				VK_SHARING_MODE_EXCLUSIVE,
				usageFlags,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
				&newDynamicBuffer->buffers[i],
				bufferSize
			);
		}

		newDynamicBuffer->map();

		_dinamicGPUBuffers.emplace(size, newDynamicBuffer);
		return newDynamicBuffer;
	}

	uint32_t VulkanRenderer::updateDynamicBufferData(VulkanDynamicBuffer* dynamicBuffer, const void* data, const uint32_t offset, const uint32_t size, const bool allBuffers, const uint32_t knownOffset) const {
		if (dynamicBuffer == nullptr) return 0;
		// todo: think about max count of mapped dynamic buffers
		const uint32_t bufferOffset = dynamicBuffer->alignedSize * (knownOffset == 0xffffffff ? dynamicBuffer->getCurrentOffset() : knownOffset);
		//dynamicBuffer->map();
		if (allBuffers) {
            for (auto&& memory : dynamicBuffer->memory) {
                memcpy(reinterpret_cast<void*>(reinterpret_cast<size_t>(memory) + bufferOffset + offset), data, size);
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

	void VulkanRenderer::bindDynamicStorageBufferToDescriptorSet(const VulkanDescriptorSet* descriptorSet, VulkanDynamicBuffer* dynamicBuffer, const uint32_t binding) const {
		bindBufferToDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, dynamicBuffer->buffers.data(), binding, dynamicBuffer->alignedSize, 0);
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
				_vulkanDevice->destroyRenderPass(_mainContinueRenderPass);
				_frameBuffers.clear();

				delete _emptyTexture;
				delete _emptyTextureArray;

                for (auto&& [size, buffer] : _dinamicGPUBuffers) {
                    buffer->unmap();
                    delete buffer;
                }
				_dinamicGPUBuffers.clear();

                for (auto&& [key, pipeline] : _graphicsPipelinesCache) {
                    vkDestroyPipeline(_vulkanDevice->device, pipeline->pipeline, nullptr);
                    delete pipeline;
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

				for (auto&& descriptorPool : _globalDescriptorPools) {
					vkDestroyDescriptorPool(_vulkanDevice->device, descriptorPool, nullptr);
				}

				_depthStencil.destroy();

				for (auto&& semaphore : _presentCompleteSemaphores) {
					semaphore.destroy();
				}
				_presentCompleteSemaphores.clear();

				for (auto&& [key, sampler] : _samplers) {
					vkDestroySampler(_vulkanDevice->device, sampler, nullptr);
				}
				_samplers.clear();

				// clear tmp frame data
				std::vector<std::vector<VulkanBuffer*>> buffersToDelete;
                std::vector<std::vector<VulkanTexture*>> texturesToDelete;
				std::vector<std::tuple<VulkanTexture*, VulkanBuffer*, uint32_t, uint32_t>> defferedTextureToGenerate;
				{
					engine::AtomicLock lock(_lockTmpData);
                    buffersToDelete = std::move(_buffersToDelete);
                    texturesToDelete = std::move(_texturesToDelete);
					defferedTextureToGenerate = std::move(_defferedTextureToGenerate);
                    _buffersToDelete.clear();
                    _texturesToDelete.clear();
					_defferedTextureToGenerate.clear();
				}

                for (auto&& buffers : buffersToDelete) {
					for (auto* buffer : buffers) {
						delete buffer;
					}
				}

                for (auto&& textures : texturesToDelete) {
                    for (auto* texture : textures) {
                        delete texture;
                    }
                }

				for (auto&& p : defferedTextureToGenerate) {
					delete std::get<VulkanBuffer*>(p);
				}

                buffersToDelete.clear();
                texturesToDelete.clear();
				defferedTextureToGenerate.clear();

				delete _vulkanDevice;
				_vulkanDevice = nullptr;
			}

			vkDestroyInstance(_instance, nullptr);
			_instance = VK_NULL_HANDLE;
		}
	}
}