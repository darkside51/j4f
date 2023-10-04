#pragma once
// ♣♠♦♥

#include "vkDescriptorSet.h"
#include "vkDynamicBuffer.h"
#include <vulkan/vulkan.h>

#include "../../Core/Hash.h"

#include <cstddef>
#include <vector>
#include <cstdint>
#include <fstream>
#include <unordered_map>
#include <string>
#include <string_view>

namespace vulkan {
	struct PipelineDescriptorLayout {
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	};

    using VulkanShaderCode = std::vector<std::byte>;
    inline VulkanShaderCode makeVulkanShaderCode(const std::byte* data, const size_t size) {
        VulkanShaderCode code;
        code.assign(data, data + size);
        return code;
    }

	struct ShaderStageInfo {
		VkShaderStageFlagBits pipelineStage;
		const char* modulePass = nullptr;
        const VulkanShaderCode* shaderCode = nullptr;
		VkSpecializationInfo* specializationInfo = nullptr;
		ShaderStageInfo() = default;
	};

	enum class GPUParamLayoutType : uint8_t { // порядок важен, будет для сортировки потом
		PUSH_CONSTANT			= 0u,
		COMBINED_IMAGE_SAMPLER	= 1u,
		UNIFORM_BUFFER			= 2u,
		STORAGE_BUFFER			= 3u,
		UNIFORM_BUFFER_DYNAMIC	= 4u,
		STORAGE_BUFFER_DYNAMIC	= 5u,
		BUFFER_PART				= 6u,
		BUFFER_DYNAMIC_PART		= 7u,
		PUSH_CONSTANT_PART		= 8u
	};

	class VulkanRenderer;

	//////////////////////
	struct VulkanUniformInfo {
		std::string name;
		uint32_t offset = 0u;
		uint32_t sizeInBytes = 0u;
	};

	enum class ImageType : uint8_t {
		sampler1D = 0u,
		sampler1D_ARRAY = 1u,
		sampler2D = 2u,
		sampler2D_ARRAY = 3u,
		sampler3D = 4u,
		samplerCube = 5u,
		samplerCube_ARRAY = 6u,
		undefined = 0xffu
	};

	struct VulkanDescriptorSetLayoutBindingDescription {
		std::string name;
		uint8_t set = 0u;
		VkDescriptorSetLayoutBinding binding;
		uint32_t sizeInBytes = 0u;
		ImageType imageType = ImageType::undefined;
		std::vector<VulkanUniformInfo> childInfos;
	};

	struct VulkanPushConstantRangeInfo {
		std::string name;
		VkPushConstantRange range;
		std::vector<VulkanUniformInfo> childInfos;
	};

	struct GPUParamLayoutInfo {
		uint8_t id = 0u;
		uint32_t set = 0u;
		uint32_t offset = 0u;
		uint32_t sizeInBytes = 0u;
		VkDescriptorSetLayoutBinding* descriptorSetLayoutBinding = nullptr;
		VkPushConstantRange* pcRange = nullptr;
		GPUParamLayoutType type = GPUParamLayoutType::PUSH_CONSTANT;
		uint32_t push_constant_number = 0u;
		uint32_t dynamicBufferIdx = 0u; // для смещения в нужном dynamic буффере при установке значения
		ImageType imageType = ImageType::undefined;
		void* data = nullptr;
		const GPUParamLayoutInfo* parentLayout = nullptr;

		[[nodiscard]] inline const void* getData() const {
			if (parentLayout) { 
				return parentLayout->getData();
			}
			return data;
		}
	};


	struct VulkanShaderModule {
		static VulkanShaderCode loadSpirVCode(const char* pass);

		VulkanShaderModule(VulkanRenderer* renderer, const ShaderStageInfo& stageInfo);
        VulkanShaderModule(VulkanRenderer* renderer, const VulkanShaderCode& code, const VkShaderStageFlagBits pipelineStage);

		~VulkanShaderModule();

		void reflectShaderCode(
			VulkanRenderer* renderer,
			const VulkanShaderCode& code
		);

		VulkanRenderer* m_renderer;
		VkShaderModule m_module;
		VkShaderStageFlagBits m_stage;
		int16_t m_maxSet = -1;

		std::vector<VulkanPushConstantRangeInfo> m_pushConstantsRange; // only one per stage
		std::vector<std::vector<VulkanDescriptorSetLayoutBindingDescription*>> m_descriptorSetLayoutBindings; // наборы дескрипторов внутри сетов
	};

	class VulkanGpuProgram {
		using params_type = std::unordered_map<std::string, GPUParamLayoutInfo*, engine::String_hash, std::equal_to<>>;
		friend class VulkanRenderer;
	public:
		inline static constexpr uint32_t UNDEFINED = 0xffffffff;

		VulkanGpuProgram(VulkanRenderer* renderer, std::vector<ShaderStageInfo>& stages, bool isParamsOwner = true);
		~VulkanGpuProgram();

		inline uint16_t getId() const noexcept { return m_id; }

//		VulkanDescriptorSet* allocateDescriptorSet(const bool add);
//		VulkanPushConstant* allocatePushConstants(const bool add);
		
		inline const VulkanDescriptorSet* getDescriptorSet(const uint32_t i) const noexcept {
			if (m_descriptorSets.empty()) return nullptr;
			return m_descriptorSets[i % m_descriptorSets.size()];
		}

		inline VkPipelineLayout getPipeLineLayout() const { return m_pipelineDescriptorLayout->pipelineLayout; }

		const GPUParamLayoutInfo* getGPUParamLayoutByName(std::string_view name) const noexcept {
			auto&& it = m_paramLayouts.find(name);
			if (it != m_paramLayouts.end()) { return it->second; }
			return nullptr;
		}

		uint32_t setValueToLayout(const GPUParamLayoutInfo* paramLayout, const void* value, VulkanPushConstant* pConstant, const uint32_t knownOffset = UNDEFINED, const uint32_t knownSize = UNDEFINED, const bool allBuffers = false);
		uint32_t setValueByName(std::string_view name, const void* value, VulkanPushConstant* pConstant, const uint32_t knownOffset = UNDEFINED, const uint32_t knownSize = UNDEFINED, const bool allBuffers = false) {
			if (const GPUParamLayoutInfo* paramLayout = getGPUParamLayoutByName(name)) {
				return setValueToLayout(paramLayout, value, pConstant, knownOffset, knownSize, allBuffers);
			}
			return UNDEFINED;
		}
		void finishUpdateParams();

		inline VulkanDynamicBuffer* getDynamicGPUBuffer(const size_t i) { return m_dynamicGPUBuffers[i]; }
		inline const VulkanDynamicBuffer* getDynamicGPUBuffer(const size_t i) const { return m_dynamicGPUBuffers[i]; }

		inline uint8_t getGPUSetsNumbers() const noexcept { return m_gpuBuffersSets; }
		inline uint8_t getGPUSetsCount(uint8_t* setsCount = nullptr) const noexcept {
			if (setsCount) {
				*setsCount = m_maxSetNum + 1;
			}
			return m_gpuBuffersSetsCount;
		}

		inline uint16_t getPushConstantsCount() const { return m_pushConstantsCount; }

		inline uint8_t getGPUBuffersSetsTypes() const { return m_gpuBuffersSetsTypes; } // типы буферов, 0 - статический(UNIFORM_BUFFER или STORAGE_BUFFER), 1 - динамический(UNIFORM_BUFFER_DYNAMIC или STORAGE_BUFFER_DYNAMIC)

		inline const params_type& getParamLayouts() const { return m_paramLayouts; }
		inline const std::vector<GPUParamLayoutInfo*>& getParamsLayoutVec() const { return m_paramLayoutsVec; }

	private:
		void parseModules(const std::vector<VulkanShaderModule*>& modules);
		void assignParams();

		uint16_t m_id;
		int16_t m_maxSetNum = -1;
		uint16_t m_dynamicBuffersCount = 0u;
		uint16_t m_pushConstantsCount = 0u;
		uint8_t m_gpuBuffersSets = 0u;
		uint8_t m_gpuBuffersSetsCount = 0u;
		uint8_t m_gpuBuffersSetsTypes = 0u;
		uint8_t m_externalDescriptors = 0u;
		VulkanRenderer* m_renderer;
		std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
		PipelineDescriptorLayout* m_pipelineDescriptorLayout = nullptr;

		params_type m_paramLayouts;
		std::vector<GPUParamLayoutInfo*> m_paramLayoutsVec;

		std::vector<VulkanDescriptorSet*> m_descriptorSets;
		std::vector<VkPushConstantRange*> m_pushConstantsRanges;

		std::vector<VulkanDynamicBuffer*> m_dynamicGPUBuffers; // bind for m_descriptorSets[i]
		std::vector<vulkan::VulkanBuffer> m_staticGPUBuffers;
	};
}