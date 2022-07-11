#pragma once

#include "vkDescriptorSet.h"
#include "vkDynamicBuffer.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
#include <fstream>
#include <unordered_map>
#include <string>

namespace vulkan {

	struct PipelineDescriptorLayout {
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	};

	struct VulkanShaderCode {
		char* shaderCode = nullptr;
		size_t shaderSize = 0;

		~VulkanShaderCode() {
			if (shaderCode) {
				delete[] shaderCode;
				shaderCode = nullptr;
				shaderSize = 0;
			}
		}

		VulkanShaderCode() = default;

		VulkanShaderCode(VulkanShaderCode&& code) noexcept {
			shaderSize = code.shaderSize;
			shaderCode = std::move(code.shaderCode);
			code.shaderCode = nullptr;
		}

		VulkanShaderCode(const VulkanShaderCode& code) {
			shaderSize = code.shaderSize;
			shaderCode = code.shaderCode;
		}

		VulkanShaderCode& operator=(const VulkanShaderCode& code) {
			shaderSize = code.shaderSize;
			shaderCode = code.shaderCode;
			return *this;
		}

		VulkanShaderCode& operator=(VulkanShaderCode&& code) noexcept {
			shaderSize = code.shaderSize;
			shaderCode = std::move(code.shaderCode);
			code.shaderCode = nullptr;
			return *this;
		}
	};

	struct ShaderStageInfo {
		VkShaderStageFlagBits pipelineStage;
		const char* modulePass;
		VkSpecializationInfo* specializationInfo;
		ShaderStageInfo() = default;
		ShaderStageInfo(VkShaderStageFlagBits s, const char* p, VkSpecializationInfo* si = nullptr) : pipelineStage(s), modulePass(p), specializationInfo(si) {}
	};

	enum class GPUParamLayoutType : uint8_t { // порядок важен, будет для сортировки потом
		PUSH_CONSTANT			= 0,
		COMBINED_IMAGE_SAMPLER	= 1,
		UNIFORM_BUFFER			= 2,
		STORAGE_BUFFER			= 3,
		UNIFORM_BUFFER_DYNAMIC	= 4,
		STORAGE_BUFFER_DYNAMIC	= 5,
		BUFFER_PART				= 6,
		BUFFER_DYNAMIC_PART		= 7,
		PUSH_CONSTANT_PART		= 8
	};

	struct GPUParamLayout {
		uint32_t offset;
		uint32_t sizeInBytes;
		uint32_t set;
		uint32_t descriptorSetBinding;
		uint32_t pushConstant;
		GPUParamLayoutType type;
		uint32_t dynamcBufferIdx;
		void* data;
		const GPUParamLayout* parentLayout = nullptr;

		const void* getData() const {
			if (parentLayout) { return parentLayout->getData(); }
			return data;
		}
	};

	class VulkanRenderer;

	struct VulkanDescriptorSetLayoutBindings {
		uint8_t set;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
	};

	
	//////////////////////
	struct VulkanUniformInfo {
		std::string name;
		uint32_t offset = 0;
		uint32_t sizeInBytes = 0;
	};

	struct VulkanDescriptorSetLayoutBindingDescription {
		std::string name;
		uint8_t set = 0;
		VkDescriptorSetLayoutBinding binding;
		uint32_t sizeInBytes = 0;
		std::vector<VulkanUniformInfo> childInfos;
	};

	struct VulkanPushConstantRangeInfo {
		std::string name;
		VkPushConstantRange range;
		std::vector<VulkanUniformInfo> childInfos;
	};

	struct GPUParamLayoutInfo {
		uint8_t id;
		uint32_t set;
		uint32_t offset;
		uint32_t sizeInBytes;
		VkDescriptorSetLayoutBinding* descriptorSetLayoutBinding;
		VkPushConstantRange* pcRange;
		GPUParamLayoutType type;
		uint32_t push_constant_number;
		uint32_t dynamcBufferIdx; // для смещения в нужном dynamic буффере при установке значения
		void* data;
		const GPUParamLayoutInfo* parentLayout = nullptr;

		inline const void* getData() const {
			if (parentLayout) { 
				return parentLayout->getData();
			}

			return data;
		}
	};


	class VulkanShaderModule {
	public:
		static VulkanShaderCode loadSpirVCode(const char* pass);

		VulkanShaderModule(VulkanRenderer* renderer, const ShaderStageInfo& stageInfo);
		~VulkanShaderModule();

	//private:
		void reflectShaderCode(
			VulkanRenderer* renderer,
			const VulkanShaderCode& code
		);

		VulkanRenderer* m_renderer;
		VkShaderModule module;
		VkShaderStageFlagBits stage;
		int16_t m_maxSet = -1;

		std::vector<VulkanPushConstantRangeInfo> m_pushConstantsRange; // only one per stage
		std::vector<std::vector<VulkanDescriptorSetLayoutBindingDescription*>> m_descriptorSetLayoutBindings; // наборы дескрипторов внутри сетов
	};

	class VulkanGpuProgram {
		friend class VulkanRenderer;
	public:
		inline static constexpr uint32_t UNDEFINED = 0xffffffff;

		VulkanGpuProgram(VulkanRenderer* renderer, std::vector<ShaderStageInfo>& stages);
		~VulkanGpuProgram();

		inline uint16_t getId() const { return m_id; }

		VulkanDescriptorSet* allocateDescriptorSet(const bool add);
		VulkanPushConstant* allocatePushConstants(const bool add);
		
		inline const VulkanDescriptorSet* getDescriptorSet(const uint32_t i) const {
			if (m_descriptorSets.empty()) return nullptr;
			return m_descriptorSets[i % m_descriptorSets.size()];
		}

		inline VkPipelineLayout getPipeLineLayout() const { return m_pipelineDescriptorLayout->pipelineLayout; }

		const GPUParamLayoutInfo* getGPUParamLayoutByName(const std::string& name) const {
			auto&& it = m_paramLayouts.find(name);
			if (it != m_paramLayouts.end()) { return it->second; }
			return nullptr;
		}

		uint32_t setValueToLayout(const GPUParamLayoutInfo* paramLayout, const void* value, VulkanPushConstant* pConstant, const uint32_t knownOffset = UNDEFINED, const uint32_t knownSize = UNDEFINED, const bool allBuffers = false);
		void finishUpdateParams();

		VulkanDynamicBuffer* getDinamicUniformBuffer(const size_t i) {
			return m_dynamicUniformBuffers[i];
		}

		inline uint8_t getGPUSetsNumbers() const { return m_gpuBuffersSets; }
		inline uint8_t getGPUSetsCount(uint8_t* setsCount = nullptr) const { 
			if (setsCount) {
				*setsCount = m_maxSetNum + 1;
			}
			return m_gpuBuffersSetsCount;
		}

		inline uint16_t getPushConstantsCount() const { return m_pushConstantsCount; }

		inline uint8_t getGPUBuffersSetsTypes() const { return m_gpuBuffersSetsTypes; } // типы буферов, 0 - статический(UNIFORM_BUFFER или STORAGE_BUFFER), 1 - динамический(UNIFORM_BUFFER_DYNAMIC или STORAGE_BUFFER_DYNAMIC)

		inline const std::unordered_map<std::string, GPUParamLayoutInfo*>& getParamLayouts() const { return m_paramLayouts; }
		inline const std::vector<GPUParamLayoutInfo*>& getParamsLayoutSortedVec() const { return m_paramLayoutsVec; }

	private:
		void parseModules(const std::vector<VulkanShaderModule*>& modules);
		void assignParams();

		uint16_t m_id;
		int16_t m_maxSetNum = -1;
		uint16_t m_dynamicBuffersCount = 0;
		uint16_t m_pushConstantsCount = 0;
		uint8_t m_gpuBuffersSets = 0;
		uint8_t m_gpuBuffersSetsCount = 0;
		uint8_t m_gpuBuffersSetsTypes = 0;
		uint8_t m_externalDescriptors = 0;
		VulkanRenderer* m_renderer;
		std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
		PipelineDescriptorLayout* m_pipelineDescriptorLayout = nullptr;

		std::unordered_map<std::string, GPUParamLayoutInfo*> m_paramLayouts;
		std::vector<GPUParamLayoutInfo*> m_paramLayoutsVec;

		std::vector<VulkanDescriptorSet*> m_descriptorSets;
		std::vector<VkPushConstantRange*> m_pushConstantsRanges;

		std::vector<VulkanDynamicBuffer*> m_dynamicUniformBuffers; // bind for m_descriptorSets[i]
		std::vector<VulkanDynamicBuffer*> m_dynamicStorageBuffers; // bind for m_descriptorSets[i]

		std::vector<vulkan::VulkanBuffer> m_staticUniformBuffers;
	};
}