#pragma once
// ♣♠♦♥

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
			shaderCode = code.shaderCode;
			code.shaderCode = nullptr;
		}

		VulkanShaderCode(const VulkanShaderCode& code) {
			shaderSize = code.shaderSize;
			shaderCode = code.shaderCode;
		}

		VulkanShaderCode& operator=(const VulkanShaderCode& code) {
            if (&code == this) return *this;
			shaderSize = code.shaderSize;
			shaderCode = code.shaderCode;
			return *this;
		}

		VulkanShaderCode& operator=(VulkanShaderCode&& code) noexcept {
			shaderSize = code.shaderSize;
			shaderCode = code.shaderCode;
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
		uint32_t offset = 0;
		uint32_t sizeInBytes = 0;
		uint32_t set = 0;
		uint32_t descriptorSetBinding = 0;
		uint32_t pushConstant = 0;
		GPUParamLayoutType type = GPUParamLayoutType::PUSH_CONSTANT;
		uint32_t dynamcBufferIdx = 0;
		void* data = nullptr;
		const GPUParamLayout* parentLayout = nullptr;

		[[nodiscard]] const void* getData() const {
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

	enum class ImageType : uint8_t {
		sampler1D = 0,
		sampler1D_ARRAY = 1,
		sampler2D = 2,
		sampler2D_ARRAY = 3,
		sampler3D = 4,
		samplerCube = 5,
		samplerCube_ARRAY = 6,
		undefined = 0xff
	};

	struct VulkanDescriptorSetLayoutBindingDescription {
		std::string name;
		uint8_t set = 0;
		VkDescriptorSetLayoutBinding binding;
		uint32_t sizeInBytes = 0;
		ImageType imageType = ImageType::undefined;
		std::vector<VulkanUniformInfo> childInfos;
	};

	struct VulkanPushConstantRangeInfo {
		std::string name;
		VkPushConstantRange range;
		std::vector<VulkanUniformInfo> childInfos;
	};

	struct GPUParamLayoutInfo {
		uint8_t id = 0;
		uint32_t set = 0;
		uint32_t offset = 0;
		uint32_t sizeInBytes = 0;
		VkDescriptorSetLayoutBinding* descriptorSetLayoutBinding = nullptr;
		VkPushConstantRange* pcRange = nullptr;
		GPUParamLayoutType type = GPUParamLayoutType::PUSH_CONSTANT;
		uint32_t push_constant_number = 0;
		uint32_t dynamcBufferIdx = 0; // для смещения в нужном dynamic буффере при установке значения
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

//		VulkanDescriptorSet* allocateDescriptorSet(const bool add);
//		VulkanPushConstant* allocatePushConstants(const bool add);
		
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
		uint32_t setValueByName(const std::string& name, const void* value, VulkanPushConstant* pConstant, const uint32_t knownOffset = UNDEFINED, const uint32_t knownSize = UNDEFINED, const bool allBuffers = false) {
			if (const GPUParamLayoutInfo* paramLayout = getGPUParamLayoutByName(name)) {
				return setValueToLayout(paramLayout, value, pConstant, knownOffset, knownSize, allBuffers);
			}
			return UNDEFINED;
		}
		void finishUpdateParams();

		inline VulkanDynamicBuffer* getDinamicGPUBuffer(const size_t i) { return m_dynamicGPUBuffers[i]; }
		inline const VulkanDynamicBuffer* getDinamicGPUBuffer(const size_t i) const { return m_dynamicGPUBuffers[i]; }

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
		inline const std::vector<GPUParamLayoutInfo*>& getParamsLayoutVec() const { return m_paramLayoutsVec; }

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

		std::vector<VulkanDynamicBuffer*> m_dynamicGPUBuffers; // bind for m_descriptorSets[i]
		std::vector<vulkan::VulkanBuffer> m_staticGPUBuffers;
	};
}