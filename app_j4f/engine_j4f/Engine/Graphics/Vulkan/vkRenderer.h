#pragma once

#include <vulkan/vulkan.h>

#include "vkDevice.h"
#include "vkSwapChain.h"
#include "vkFrameBuffer.h"
#include "vkPipeline.h"
#include "vkImage.h"
#include "vkSynchronisation.h"
#include "vkGPUProgram.h"
#include "vkDescriptorSet.h"
#include "vkDynamicBuffer.h"

#include "../../Core/Threads/Synchronisations.h"
#include "../../Engine/Core/Hash.h"
#include "../../Engine/Core/Version.h"

#include <cstdint>
#include <vector>
#include <map>
#include <memory>
#include <unordered_map>
#include <atomic>
#include <utility>
#include <tuple>

namespace engine {
	class IRenderSurfaceInitialiser;
	struct GraphicConfig;
}

namespace vulkan {

	enum class PrimitiveTopology : uint8_t {
		POINT_LIST = 0,
		LINE_LIST = 1,
		LINE_STRIP = 2,
		TRIANGLE_LIST = 3,
		TRIANGLE_STRIP = 4,
		TRIANGLE_FAN = 5,
		LINE_LIST_WITH_ADJACENCY = 6,
		LINE_STRIP_WITH_ADJACENCY = 7,
		TRIANGLE_LIST_WITH_ADJACENCY = 8,
		TRIANGLE_STRIP_WITH_ADJACENCY = 9,
		PATCH_LIST = 10
	};

	enum class PoligonMode : uint8_t {
		POLYGON_MODE_FILL = 0,
		POLYGON_MODE_LINE = 1,
		POLYGON_MODE_POINT = 2
	};

	enum class CullMode : uint8_t {
		CULL_MODE_NONE = 0,
		CULL_MODE_FRONT = 1,
		CULL_MODE_BACK = 2,
		CULL_MODE_FRONT_AND_BACK = 3
	};

	enum class FaceOrientation : uint8_t {
		FACE_COUNTER_CLOCKWISE = 0,
		FACE_CLOCKWISE = 1
	};

	enum class BlendFactor : uint8_t {
		BLEND_FACTOR_ZERO = 0,
		BLEND_FACTOR_ONE = 1,
		BLEND_FACTOR_SRC_COLOR = 2,
		BLEND_FACTOR_ONE_MINUS_SRC_COLOR = 3,
		BLEND_FACTOR_SRC_ALPHA = 4,
		BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = 5,
		BLEND_FACTOR_DST_ALPHA = 6,
		BLEND_FACTOR_ONE_MINUS_DST_ALPHA = 7,
		BLEND_FACTOR_DST_COLOR = 8,
		BLEND_FACTOR_ONE_MINUS_DST_COLOR = 9,
		BLEND_FACTOR_SRC_ALPHA_SATURATE = 10
	};

	enum class BlendFunction : uint8_t {
		BLEND_FUNC_ADD = 0,
		BLEND_FUNC_MIN = 1,
		BLEND_FUNC_MAX = 2,
		BLEND_FUNC_SUBTRACT = 3,
		BLEND_FUNC_REVERSE_SUBTRACT = 4
	};

	struct VulkanPrimitiveTopology {
		VulkanPrimitiveTopology() : topology(PrimitiveTopology::TRIANGLE_LIST), enableRestart(false) {}
		VulkanPrimitiveTopology(const PrimitiveTopology t, const bool er) : topology(t), enableRestart(er) {}
		PrimitiveTopology topology : 4;
		bool enableRestart : 1;
	};

	struct VulkanRasterizationState {
		PoligonMode poligonMode : 2;
		CullMode cullMode : 2;
		FaceOrientation faceOrientation : 1;
		bool discardEnable : 1;

		explicit VulkanRasterizationState(CullMode cm, PoligonMode pm = PoligonMode::POLYGON_MODE_FILL,
                                 FaceOrientation fo = FaceOrientation::FACE_COUNTER_CLOCKWISE,
                                 bool depthClampOn = false, bool discardOn = false,
                                 bool depthBiasOn = false) :
			poligonMode(pm), cullMode(cm), faceOrientation(fo), discardEnable(discardOn) { }

		[[nodiscard]] inline VkPipelineRasterizationStateCreateInfo rasterizationInfo() const {
			VkPipelineRasterizationStateCreateInfo rasterizationState = {};
			rasterizationState.sType                    = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizationState.lineWidth                = 1.0f;

			rasterizationState.polygonMode              = static_cast<VkPolygonMode>(poligonMode);
			rasterizationState.cullMode                 = static_cast<VkCullModeFlags>(cullMode);
			rasterizationState.frontFace                = static_cast<VkFrontFace>(faceOrientation);
			rasterizationState.rasterizerDiscardEnable  = discardEnable;

			rasterizationState.depthClampEnable         = VK_FALSE;
			rasterizationState.depthBiasEnable          = VK_TRUE;
			rasterizationState.depthBiasConstantFactor  = 0.0f;
			rasterizationState.depthBiasClamp           = 0.0f;
			rasterizationState.depthBiasSlopeFactor     = 0.0f;

			return rasterizationState;
		}

		inline uint8_t operator()() const { return (static_cast<uint8_t>(poligonMode) << 0) |
                                                    (static_cast<uint8_t>(cullMode) << 2) |
                                                    (static_cast<uint8_t>(faceOrientation) << 4) |
                                                    (static_cast<uint8_t>(discardEnable) << 5); } // 6 bit
	};

	struct BlendParameters {
		uint8_t useBlending : 1;
		uint8_t blendFunctionRGB : 3;
		uint8_t blendFunctionAlpha : 3;
		uint8_t srcBlendFactor : 4;
		uint8_t dstBlendFactor : 4;
		uint8_t srcAlphaBlendFactor : 4;
		uint8_t dstAlphaBlendFactor : 4;

        constexpr BlendParameters(const uint8_t useBlending, const BlendFactor srcColor, const BlendFactor dstColor,
                        const BlendFactor srcAlpha, const BlendFactor dstAlpha,
                        const BlendFunction rgbFunction, const BlendFunction alphaFunction) :
			useBlending(useBlending),
			blendFunctionRGB(static_cast<uint8_t>(rgbFunction)),
			blendFunctionAlpha(static_cast<uint8_t>(alphaFunction)),
			srcBlendFactor(static_cast<uint8_t>(srcColor)),
			dstBlendFactor(static_cast<uint8_t>(dstColor)),
			srcAlphaBlendFactor(static_cast<uint8_t>(srcAlpha)),
			dstAlphaBlendFactor(static_cast<uint8_t>(dstAlpha)) {}

        constexpr BlendParameters(const BlendFactor srcColor, const BlendFactor dstColor,
                        const BlendFactor srcAlpha, const BlendFactor dstAlpha) :
			useBlending(1),
			blendFunctionRGB(static_cast<uint8_t>(BlendFunction::BLEND_FUNC_ADD)),
			blendFunctionAlpha(static_cast<uint8_t>(BlendFunction::BLEND_FUNC_ADD)),
			srcBlendFactor(static_cast<uint8_t>(srcColor)),
			dstBlendFactor(static_cast<uint8_t>(dstColor)),
			srcAlphaBlendFactor(static_cast<uint8_t>(srcAlpha)),
			dstAlphaBlendFactor(static_cast<uint8_t>(dstAlpha)) {}

        constexpr BlendParameters(const BlendFactor src, const BlendFactor dst) :
			useBlending(1),
			blendFunctionRGB(static_cast<uint8_t>(BlendFunction::BLEND_FUNC_ADD)),
			blendFunctionAlpha(static_cast<uint8_t>(BlendFunction::BLEND_FUNC_ADD)),
			srcBlendFactor(static_cast<uint8_t>(src)),
			dstBlendFactor(static_cast<uint8_t>(dst)),
			srcAlphaBlendFactor(static_cast<uint8_t>(src)),
			dstAlphaBlendFactor(static_cast<uint8_t>(dst)) {}
	};

	union VulkanBlendMode {
        constexpr explicit VulkanBlendMode(const uint32_t mode = 0u) noexcept : _blendMode(mode) {};
        constexpr VulkanBlendMode(VulkanBlendMode&& mode) noexcept : _blendMode(mode._blendMode) {};
        constexpr VulkanBlendMode(const VulkanBlendMode& mode) : _blendMode(mode._blendMode) {};
        constexpr explicit VulkanBlendMode(const BlendParameters& parameters) : _parameters(parameters) {};
        constexpr explicit VulkanBlendMode(BlendParameters&& parameters) noexcept : _parameters(std::move(parameters)) {};

		template <class ...Args>
		static inline constexpr VulkanBlendMode makeBlendMode(Args&&...args) {
            return VulkanBlendMode(BlendParameters(std::forward<Args&&>(args)...));
        }

		VulkanBlendMode& operator=(const VulkanBlendMode& m) {
			_blendMode = m._blendMode;
			return *this;
		}

        VulkanBlendMode& operator=(VulkanBlendMode&& m) noexcept {
            _blendMode = m._blendMode;
            return *this;
        }

		inline constexpr uint32_t operator()() const noexcept { return _blendMode; }

		[[nodiscard]] inline std::vector<VkPipelineColorBlendAttachmentState> blendState() const {
			std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentState(1);

			blendAttachmentState[0].colorWriteMask = 0xf;
			blendAttachmentState[0].blendEnable = _parameters.useBlending ? VK_TRUE : VK_FALSE;

			blendAttachmentState[0].srcColorBlendFactor = convertBlendFactor(static_cast<BlendFactor>(_parameters.srcBlendFactor));
			blendAttachmentState[0].dstColorBlendFactor = convertBlendFactor(static_cast<BlendFactor>(_parameters.dstBlendFactor));
			blendAttachmentState[0].srcAlphaBlendFactor = convertBlendFactor(static_cast<BlendFactor>(_parameters.srcAlphaBlendFactor));
			blendAttachmentState[0].dstAlphaBlendFactor = convertBlendFactor(static_cast<BlendFactor>(_parameters.dstAlphaBlendFactor));

			blendAttachmentState[0].colorBlendOp = convertBlendOp(static_cast<BlendFunction>(_parameters.blendFunctionRGB));
			blendAttachmentState[0].alphaBlendOp = convertBlendOp(static_cast<BlendFunction>(_parameters.blendFunctionAlpha));

			return blendAttachmentState;
		}

	private:
		static inline constexpr VkBlendFactor convertBlendFactor(const BlendFactor f) noexcept {
			switch (f) {
                case BlendFactor::BLEND_FACTOR_ZERO:
					return VK_BLEND_FACTOR_ZERO;
				case BlendFactor::BLEND_FACTOR_ONE:
					return VK_BLEND_FACTOR_ONE;
				case BlendFactor::BLEND_FACTOR_SRC_COLOR:
					return VK_BLEND_FACTOR_SRC_COLOR;
				case BlendFactor::BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
					return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
				case BlendFactor::BLEND_FACTOR_SRC_ALPHA:
					return VK_BLEND_FACTOR_SRC_ALPHA;
				case BlendFactor::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
					return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				case BlendFactor::BLEND_FACTOR_DST_ALPHA:
					return VK_BLEND_FACTOR_DST_ALPHA;
				case BlendFactor::BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
					return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
				case BlendFactor::BLEND_FACTOR_DST_COLOR:
					return VK_BLEND_FACTOR_DST_COLOR;
				case BlendFactor::BLEND_FACTOR_ONE_MINUS_DST_COLOR:
					return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
				case BlendFactor::BLEND_FACTOR_SRC_ALPHA_SATURATE:
					return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
				default:
					return VK_BLEND_FACTOR_ZERO;
			}
		}

		static inline constexpr VkBlendOp convertBlendOp(const BlendFunction f) noexcept {
			switch (f) {
                case BlendFunction::BLEND_FUNC_ADD:
					return VK_BLEND_OP_ADD;
				case BlendFunction::BLEND_FUNC_MIN:
					return VK_BLEND_OP_MIN;
				case BlendFunction::BLEND_FUNC_MAX:
					return VK_BLEND_OP_MAX;
				case BlendFunction::BLEND_FUNC_SUBTRACT:
					return VK_BLEND_OP_SUBTRACT;
				case BlendFunction::BLEND_FUNC_REVERSE_SUBTRACT:
					return VK_BLEND_OP_REVERSE_SUBTRACT;
				default:
					return VK_BLEND_OP_ADD;
			}
		}

		BlendParameters _parameters;
		uint32_t _blendMode = 0;
	};

	namespace CommonBlendModes {
        // normal modes
        [[maybe_unused]] inline const static VulkanBlendMode blend_none	    = VulkanBlendMode(0u);
        [[maybe_unused]] inline const static VulkanBlendMode blend_alpha	= VulkanBlendMode::makeBlendMode(BlendFactor::BLEND_FACTOR_SRC_ALPHA, BlendFactor::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
        [[maybe_unused]] inline const static VulkanBlendMode blend_add		= VulkanBlendMode::makeBlendMode(BlendFactor::BLEND_FACTOR_SRC_ALPHA, BlendFactor::BLEND_FACTOR_ONE);
        [[maybe_unused]] inline const static VulkanBlendMode blend_multiply	= VulkanBlendMode::makeBlendMode(BlendFactor::BLEND_FACTOR_DST_COLOR, BlendFactor::BLEND_FACTOR_ZERO);
        // premultily alpha modes
        // https://gamedev.ru/code/articles/PremultipliedAlpha
        // https://habr.com/ru/company/playrix/blog/336304/
        // https://apoorvaj.io/alpha-compositing-opengl-blending-and-premultiplied-alpha/
        [[maybe_unused]] inline const static VulkanBlendMode blend_alpha_pma   = VulkanBlendMode::makeBlendMode(BlendFactor::BLEND_FACTOR_ONE, BlendFactor::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
        [[maybe_unused]] inline const static VulkanBlendMode blend_add_pma     = VulkanBlendMode::makeBlendMode(BlendFactor::BLEND_FACTOR_ONE, BlendFactor::BLEND_FACTOR_ONE);
	};

	struct VulkanStencilState {
		bool enabled : 1;
		VkStencilOp failOp;
		VkStencilOp passOp;
		VkStencilOp depthFailOp;
		VkCompareOp compareOp;

		uint8_t compareMask;
		uint8_t writeMask;
		uint8_t reference;

		explicit VulkanStencilState(
			const bool on = false,
			const VkStencilOp fail = VK_STENCIL_OP_KEEP,
			const VkStencilOp pass = VK_STENCIL_OP_KEEP,
			const VkStencilOp depthFail = VK_STENCIL_OP_KEEP,
			const VkCompareOp cmpOp = VK_COMPARE_OP_ALWAYS,
			const uint8_t cmpMask = 0xff,
			const uint8_t wrtMask = 0xff,
			const uint8_t ref = 1
		) :
			enabled(on),
			failOp(fail),
			passOp(pass),
			depthFailOp(depthFail),
			compareOp(cmpOp),
			compareMask(cmpMask),
			writeMask(wrtMask),
			reference(ref)
		{ }

		inline bool operator == (const VulkanStencilState& other) const noexcept {
			return (enabled == other.enabled)		&&
				(failOp == other.failOp)			&&
				(passOp == other.passOp)			&&
				(compareOp == other.compareOp)		&&
				(depthFailOp == other.depthFailOp)	&&
				(compareMask == other.compareMask)	&&
				(writeMask == other.writeMask)		&&
				(reference == other.reference);
		}

		bool operator < (const VulkanStencilState& other) const noexcept {
            return std::tie(enabled, failOp, passOp, compareOp,
                     depthFailOp, compareMask, writeMask, reference) <
            std::tie(other.enabled, other.failOp, other.passOp, other.compareOp,
                     other.depthFailOp, other.compareMask, other.writeMask, other.reference);
		}
	};

	struct VulkanDepthState {
		VkCompareOp compareOp;
		bool depthTestEnabled;
		bool depthWriteEnabled;
		explicit VulkanDepthState(
			const bool testOn,
			const bool writeOn,
			VkCompareOp cmpOp = VK_COMPARE_OP_LESS
		) : 
			compareOp(cmpOp),
			depthTestEnabled(testOn),
			depthWriteEnabled(writeOn)
		{ }
	};

	struct VertexDescription {
		std::vector<std::pair<uint32_t, uint32_t>> bindings_strides{}; // pair of binding, stride
		uint32_t attributesCount = 0;
		VkVertexInputAttributeDescription* attributes = nullptr;
	};

	struct VulkanRenderState {
		VulkanRenderState() : depthState(false, false), rasterizationState(CullMode::CULL_MODE_BACK) {}

		VertexDescription vertexDescription;
		VulkanDepthState depthState;
		VulkanStencilState stencilState;
		VulkanBlendMode blendMode;
		VulkanRasterizationState rasterizationState;
		VulkanPrimitiveTopology topology;
	};

	class VulkanTexture;

	class VulkanRenderer {
	public:
		VulkanRenderer();
		~VulkanRenderer();

		PipelineDescriptorLayout& getDescriptorLayout(const std::vector<std::vector<VkDescriptorSetLayoutBinding*>>& setLayoutBindings, const std::vector<VkPushConstantRange*>& pushConstantsRanges);

		VulkanDescriptorSet* allocateDescriptorSetFromGlobalPool(const VkDescriptorSetLayout descriptorLayout, const uint32_t count = 0);
		std::pair<VkDescriptorSet, uint32_t> allocateSingleDescriptorSetFromGlobalPool(const VkDescriptorSetLayout descriptorSetLayout);

		void freeDescriptorSetsFromGlobalPool(const VkDescriptorSet* sets, const uint32_t count, const uint32_t poolId) const {
			vkFreeDescriptorSets(_vulkanDevice->device, _globalDescriptorPools[poolId], count, sets);
		}

		void freeDescriptorSets(const VulkanDescriptorSet* set) const {
			vkFreeDescriptorSets(_vulkanDevice->device, set->parentPool, set->set.size(), set->set.data());
		}

		void bindBufferToDescriptorSet(const VulkanDescriptorSet* descriptorSet, const VkDescriptorType type, const VulkanBuffer* buffer, const uint32_t binding, const uint32_t alignedSize, const uint32_t offset) const;
		void bindImageToDescriptorSet(
			const VulkanDescriptorSet* descriptorSet,
			const VkDescriptorType type,
			const VkSampler sampler,
			const VkImageView imageView,
			const VkImageLayout imageLayout,
			const uint32_t binding
		) const;

		void bindImageToSingleDescriptorSet(
			const VkDescriptorSet descriptorSet,
			const VkDescriptorType type,
			const VkSampler sampler,
			const VkImageView imageView,
			const VkImageLayout imageLayout,
			const uint32_t binding
		) const;

		void beginFrame();
		void endFrame();

		inline const VulkanDevice* getDevice() const noexcept { return _vulkanDevice; }
		inline VulkanDevice* getDevice() noexcept { return _vulkanDevice; }

		inline VkCommandPool getCommandPool() noexcept { return _commandPool; }
		inline VkCommandPool getCommandPool() const noexcept { return _commandPool; }

		inline VkQueue getMainQueue() noexcept{ return _mainQueue; }
		inline VkQueue getMainQueue() const noexcept { return _mainQueue; }
		 
		inline uint32_t getSwapchainImagesCount() const noexcept { return _swapChain.imageCount; }

		void resize(const uint32_t w, const uint32_t h, const bool vSync);

		VulkanPipeline* getGraphicsPipeline(
			const VulkanRenderState& renderState,
			const VulkanGpuProgram* program,
			const VkRenderPass renderPass = VK_NULL_HANDLE,
			const uint8_t subpass = 0
		);

		VulkanPipeline* getGraphicsPipeline(
			const VertexDescription& vertexDescription,
			const VulkanPrimitiveTopology& topology,
			const VulkanRasterizationState& rasterization,
			const VulkanBlendMode& blendMode,
			const VulkanDepthState& depthState,
			const VulkanStencilState& stencilState,
			const VulkanGpuProgram* program,
			const VkRenderPass renderPass = VK_NULL_HANDLE,
			const uint8_t subpass = 0
		);

		bool createInstance(
                const engine::Version apiVersion,
                const engine::Version engineVersion,
                const engine::Version applicationVersion,
                const std::vector<VkExtensionProperties>& desiredInstanceExtensions,
                const std::vector<VkLayerProperties>& desiredLayers);
		bool createDevice(const VkPhysicalDeviceFeatures features, std::vector<const char*>& extensions, const VkPhysicalDeviceType deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
		void createSwapChain(const engine::IRenderSurfaceInitializer* initializer, const bool useVsync);
		void init(const engine::GraphicConfig& cfg);

		void setupRenderPass(const std::vector<VkAttachmentDescription>& configuredAttachments, const std::vector<VkSubpassDependency>& configuredDependencies, const bool canContinueMainRenderPass);
		VkResult setupDescriptorPool(const std::vector<VkDescriptorPoolSize>& descriptorPoolCfg);

		void waitWorkComplete() const;
		void destroy();

		template <typename STATE = VulkanCommandBufferState>
		inline VulkanCommandBufferEx<STATE> createCommandBuffer(const VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const {
			return VulkanCommandBufferEx<STATE>(_vulkanDevice->device, _commandPool, level);
		}

		template <typename STATE = VulkanCommandBufferState>
		inline VulkanCommandBuffersArrayEx<STATE> createCommandBuffer(
			VkPipelineStageFlags waitStageMask,
			uint32_t cmdBuffersCount, 
			const VkSemaphoreCreateFlags signalSemFlags = 0, 
			const VkAllocationCallbacks* signalSempAllocator = nullptr, 
			const VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
		) const {
			return VulkanCommandBuffersArrayEx<STATE>(_vulkanDevice->device, _commandPool, waitStageMask, cmdBuffersCount, signalSemFlags, signalSempAllocator, level);
		}

		inline VulkanCommandBuffer& getRenderCommandBuffer() noexcept { return _mainRenderCommandBuffers[_currentFrame]; }
		inline VulkanCommandBuffer& getSupportCommandBuffer() noexcept { return _mainSupportCommandBuffers[_currentFrame]; }

		inline const VulkanFrameBuffer& getFrameBuffer() const noexcept { return _frameBuffers[_currentFrame]; }

		inline uint32_t getCurrentFrame() const noexcept { return _currentFrame; }
		inline VkRenderPass getMainRenderPass() const noexcept { return _mainRenderPass; }
		inline VkRenderPass getMainContinueRenderPass() const noexcept { return _mainContinueRenderPass; }

		//VkDescriptorPool getGlobalDescriptorPool() const { return _globalDescriptorPool; }

		// dynamic uniform buffers
		VulkanDynamicBuffer* getDynamicGPUBufferForSize(const uint32_t size, const VkBufferUsageFlags usageFlags, const uint32_t maxCount = 2048);
		uint32_t updateDynamicBufferData(VulkanDynamicBuffer* dynamicBuffer, const void* data, const uint32_t offset, const uint32_t size, const bool allBuffers = false, const uint32_t knownOffset = 0xffffffff) const;

		inline uint32_t updateDynamicBufferData(VulkanDynamicBuffer* dynamicBuffer, const void* data, const bool allBuffers = false, const uint32_t knownOffset = 0xffffffff, const uint32_t knownSize = 0xffffffff) const {
			if (dynamicBuffer == nullptr) return 0;
			const uint32_t bufferOffset = updateDynamicBufferData(dynamicBuffer, data, 0, (knownSize == 0xffffffff ? dynamicBuffer->alignedSize : knownSize), allBuffers, knownOffset);
			return bufferOffset;
		}

		void bindDynamicUniformBufferToDescriptorSet(const VulkanDescriptorSet* descriptorSet, VulkanDynamicBuffer* dynamicBuffer, const uint32_t binding) const;
		void bindDynamicStorageBufferToDescriptorSet(const VulkanDescriptorSet* descriptorSet, VulkanDynamicBuffer* dynamicBuffer, const uint32_t binding) const;

		VkSampler getSampler(
			const VkFilter minFilter,
			const VkFilter magFilter,
			const VkSamplerMipmapMode mipmapMode,
			const VkSamplerAddressMode addressModeU,
			const VkSamplerAddressMode addressModeV,
			const VkSamplerAddressMode addressModeW,
			const VkBorderColor borderColor
		);

		void markToDelete(VulkanBuffer* buffer);
        void markToDelete(VulkanTexture* texture);
        void markToDelete(VulkanTexture&& texture);

		void addDefferedGenerateTexture(VulkanTexture* t, VulkanBuffer* b, const uint32_t baseLayer, const uint32_t layerCount) {
			engine::AtomicLock lock(_lockTmpData);
			_defferedTextureToGenerate.emplace_back(t, b, baseLayer, layerCount);
		}

		inline uint32_t getWidth() const noexcept { return _width; }
		inline uint32_t getHeight() const noexcept { return _height; }
		inline uint64_t getWH() const noexcept {
			const uint64_t wh = static_cast<uint64_t>(_width) << 0 | static_cast<uint64_t>(_height) << 32;
			return wh;
		}

        inline auto getSize() const noexcept {
            return std::make_pair(_width, _height);
        }

		inline const VulkanTexture* getEmptyTexture() const noexcept { return _emptyTexture; }
		inline const VulkanTexture* getEmptyTextureArray() const noexcept { return _emptyTextureArray; }

        constexpr inline const char* getName() const noexcept { return "vulkan"; }

	private:
		VkResult allocateDescriptorSetsFromGlobalPool(VkDescriptorSetAllocateInfo& allocInfo, VkDescriptorSet* set);
		void buildDefaultMainRenderCommandBuffer();
		void createEmptyTexture();

		struct CachedDescriptorLayouts {
			std::vector<std::vector<uint32_t>> bindingsCharacterVec;
			std::vector<uint64_t> constantsCharacterVec;
			uint16_t descriptorSetLayoutsHandled;
			PipelineDescriptorLayout layout;

			CachedDescriptorLayouts(std::vector<std::vector<uint32_t>>&& bindings, std::vector<uint64_t>&& constants, PipelineDescriptorLayout&& l, const uint16_t handled) : bindingsCharacterVec(std::move(bindings)), constantsCharacterVec(std::move(constants)), layout(std::move(l)), descriptorSetLayoutsHandled(handled) { }
		};

		void* _deviceCreatepNextChain = nullptr;

		bool _vSync = false;
		uint32_t _width = 0u;
        uint32_t _height = 0u;
		uint32_t _currentFrame = 0u;
		uint32_t _swapchainImagesCount = 0u;
		uint8_t _mainDepthFormatBits = 24u;

		VkInstance _instance = VK_NULL_HANDLE;
		VulkanDevice* _vulkanDevice = nullptr;
		VulkanSwapChain _swapChain = {};
		VkPipelineCache _pipelineCache = VK_NULL_HANDLE;

		std::vector<VkDescriptorPool> _globalDescriptorPools;
		uint32_t _currentDescriptorPool = 0xffffffffu;
		std::vector<VkDescriptorPoolSize> _descriptorPoolCustomConfig;

		VkCommandPool _commandPool = VK_NULL_HANDLE;
		VkRenderPass _mainRenderPass = VK_NULL_HANDLE;
		VkRenderPass _mainContinueRenderPass = VK_NULL_HANDLE;
		std::vector<VulkanFrameBuffer> _frameBuffers;

		VkQueue _mainQueue = VK_NULL_HANDLE;
		VkQueue _presentQueue = VK_NULL_HANDLE;
		
		std::vector<VulkanSemaphore> _presentCompleteSemaphores;
		std::vector<VulkanFence> _waitFences;

		VulkanCommandBuffersArray _mainRenderCommandBuffers;
		VulkanCommandBuffersArray _mainSupportCommandBuffers;
		VkSubpassContents _mainSubpassContents = VK_SUBPASS_CONTENTS_INLINE;

		VkClearValue _mainClearValues[2];

		VulkanImage _depthStencil = {};
		VkFormat _mainDepthFormat = VK_FORMAT_MAX_ENUM;

		//// caches
		struct GraphicsPipelineCacheKey {
			uint64_t composite_key = 0;
			uint64_t stencil_key = 0;
			uint32_t blend_key = 0;
			VkRenderPass renderPass = VK_NULL_HANDLE;
			size_t hash_value = 0xffffffff;

			GraphicsPipelineCacheKey(const uint64_t cmpst, const uint64_t stncl, const uint32_t blnd, VkRenderPass rp) : 
				composite_key(cmpst),
				stencil_key(stncl),
				blend_key(blnd),
				renderPass(rp)
			{
				hash_value = engine::hash_combine(composite_key, stencil_key, blend_key, renderPass);
			}

			inline bool operator < (const GraphicsPipelineCacheKey& key) const noexcept {
				if (composite_key < key.composite_key) return true;
				if (composite_key == key.composite_key) {
					if (stencil_key < key.stencil_key) return true;
					if (stencil_key == key.stencil_key) {
						if (blend_key < key.blend_key) return true;
						if (blend_key == key.blend_key) {
							return (renderPass < key.renderPass);
						}
						return false;
					}
					return false;
				}
				return false;
			}

			inline bool operator == (const GraphicsPipelineCacheKey& key) const noexcept {
				return (composite_key == key.composite_key) && (stencil_key == key.stencil_key) && (blend_key == key.blend_key) && (renderPass == key.renderPass);
			}

			[[nodiscard]] inline size_t hash() const noexcept { return hash_value; }
		};

		std::vector<CachedDescriptorLayouts*> _descriptorLayoutsCache;
		//std::map<GraphicsPipelineCacheKey, VulkanPipeline*> _graphicsPipelinesCache;
		std::unordered_map<GraphicsPipelineCacheKey, std::unique_ptr<VulkanPipeline>, engine::Hasher<GraphicsPipelineCacheKey>> _graphicsPipelinesCache; // try using with engine::hash
		//// caches

		// dynamic buffers
		std::unordered_map<uint32_t, VulkanDynamicBuffer*> _dinamicGPUBuffers;

		// samplers
		std::unordered_map<uint16_t, VkSampler> _samplers;

		// tmp frame data
		std::atomic_bool _lockTmpData = {};
		std::vector<std::vector<VulkanBuffer*>> _buffersToDelete;
		std::vector<std::tuple<VulkanTexture*, VulkanBuffer*, uint32_t, uint32_t>> _defferedTextureToGenerate;
        std::vector<std::vector<VulkanTexture*>> _texturesToDelete;
        std::vector<std::vector<VulkanTexture>> _texturesToFree;
		// empty data
		VulkanTexture* _emptyTexture = nullptr;
		VulkanTexture* _emptyTextureArray = nullptr;
	};

}