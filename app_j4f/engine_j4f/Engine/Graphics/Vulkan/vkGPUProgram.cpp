﻿// ♣♠♦♥
#include "vkGPUProgram.h"
#include "vkDevice.h"
#include "vkHelper.h"
#include "vkRenderer.h"
#include "vkDebugMarker.h"

#include "../../Core/Cache.h"
#include "../../File/FileManager.h"

// reflect
#include "spirv/spirv_reflect.h"

// some
#include <cstdint>
#include <cassert>
#include <array>

namespace vulkan {

	void parseBuffer(const SpvReflectDescriptorBinding* binding, VulkanDescriptorSetLayoutBindingDescription* parent, uint32_t& resultDescriptorSize) {
		uint32_t elementOffset = 0;
		parent->childInfos.resize(binding->type_description->member_count);

		for (uint32_t j = 0; j < binding->type_description->member_count; ++j) {
			SpvReflectTypeDescription* member = &(binding->type_description->members[j]);

			if (member->type_flags & SPV_REFLECT_TYPE_FLAG_VOID) continue;

			// https://vulkan-tutorial.com/Uniform_buffers/Descriptor_pool_and_sets#page_Alignment-requirements
			const uint32_t oneElementSize = 4;
				//(member->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT)	? sizeof(float) :
				//(member->type_flags & SPV_REFLECT_TYPE_FLAG_INT)	? sizeof(int32_t) :
				//(member->type_flags & SPV_REFLECT_TYPE_FLAG_BOOL)	? sizeof(bool) :
				//(member->traits.numeric.scalar.width / 8); // размер элемента данных в байтах

			uint32_t size;
			switch (member->op) {
				case SpvOpTypeMatrix:
				{
					const uint32_t stride = member->traits.numeric.matrix.stride; // выравнивание элемента
					const uint32_t startMatrixStride = stride / oneElementSize; // размер выравнивания для начала матрицы
					size = member->traits.numeric.matrix.row_count * member->traits.numeric.matrix.column_count * oneElementSize;
					elementOffset = engine::alignValue(elementOffset, startMatrixStride); // матрицы должны быть выравнены по размеру элемента, иначе работает не правильно
				}
					break;
				case SpvOpTypeArray:
				{
					const uint32_t stride = member->traits.array.stride; // выравнивание элемента
					const uint32_t startArrayStride = stride / oneElementSize; // размер выравнивания для начала массива
					size = stride;
					for (size_t n = 0; n < member->traits.array.dims_count; ++n) {
						size *= member->traits.array.dims[n];
					}
					elementOffset = engine::alignValue(elementOffset, startArrayStride); // массивы должны быть выравнены по размеру элемента, иначе работает не правильно
				}
					break;
				case SpvOpTypeVector:
					// https://vulkan-tutorial.com/Uniform_buffers/Descriptor_pool_and_sets#page_Alignment-requirements
					switch (member->traits.numeric.vector.component_count) {
						case 2:
							elementOffset = engine::alignValue(elementOffset, 8);
							break;
						case 3:
						case 4:
							elementOffset = engine::alignValue(elementOffset, 16);
							break;
						default:
							break;
					}
					size = member->traits.numeric.vector.component_count * oneElementSize;
					break;
				case SpvOpTypeBool:
				case SpvOpTypeFloat:
				case SpvOpTypeInt:
				default:
					size = oneElementSize;
					break;
			}

			// сохраняем данные о смещении и размере
			if (size > 0) {
				parent->childInfos[j] = { std::string(member->struct_member_name), elementOffset, size };
				elementOffset += size;
			}
		}

		resultDescriptorSize = elementOffset;
	}

	VulkanShaderCode VulkanShaderModule::loadSpirVCode(const char* pass) {
		auto& fm = engine::Engine::getInstance().getModule<engine::FileManager>();
		VulkanShaderCode code;
		fm.readFile(pass, code);
		return code;
	}

	VulkanShaderModule::VulkanShaderModule(VulkanRenderer* renderer, const ShaderStageInfo& stageInfo) : m_renderer(renderer), m_stage(stageInfo.pipelineStage) {
		const VulkanShaderCode code = VulkanShaderModule::loadSpirVCode(stageInfo.modulePass);
		reflectShaderCode(renderer, code);
		m_module = renderer->getDevice()->createShaderModule(code);
	}

    VulkanShaderModule::VulkanShaderModule(VulkanRenderer* renderer, const VulkanShaderCode& code, const VkShaderStageFlagBits pipelineStage) :
            m_renderer(renderer), m_stage(pipelineStage) {
        reflectShaderCode(renderer, code);
        m_module = renderer->getDevice()->createShaderModule(code);
    }

	VulkanShaderModule::~VulkanShaderModule() {
		for (auto&& v : m_descriptorSetLayoutBindings) {
			for (auto* d : v) {
				delete d;
			}
		}

		m_descriptorSetLayoutBindings.clear();

		m_renderer->getDevice()->destroyShaderModule(m_module);
	}

	void VulkanShaderModule::reflectShaderCode(VulkanRenderer* renderer, const VulkanShaderCode& code) {
	
		SpvReflectShaderModule reflectedModule;
		SpvReflectResult result = spvReflectCreateShaderModule(code.size(), code.data(), &reflectedModule);
		if (result != SPV_REFLECT_RESULT_SUCCESS) {
			assert(false);
		}

		// список юниформов
		uint32_t setsCount = 0;
		spvReflectEnumerateDescriptorSets(&reflectedModule, &setsCount, nullptr);
		std::vector<SpvReflectDescriptorSet*> sets(setsCount);
		spvReflectEnumerateDescriptorSets(&reflectedModule, &setsCount, sets.data());

		m_descriptorSetLayoutBindings.resize(setsCount);

		for (uint32_t i = 0; i < setsCount; ++i) {
			const SpvReflectDescriptorSet* setInfo = sets[i];

			m_maxSet = m_maxSet > static_cast<int16_t>(setInfo->set) ? m_maxSet : static_cast<int16_t>(setInfo->set);

			for (uint32_t k = 0; k < setInfo->binding_count; ++k) {
				const SpvReflectDescriptorBinding* binding = setInfo->bindings[k];
				switch (binding->descriptor_type) {
				case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: // = VK_DESCRIPTOR_TYPE_SAMPLER
					break;
				case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: // = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
				{
					// создаем лаяут дескриптора изображения
					VkDescriptorSetLayoutBinding layoutBinding;
					layoutBinding.binding = binding->binding;
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					layoutBinding.descriptorCount = binding->count;
					layoutBinding.stageFlags = m_stage;
					layoutBinding.pImmutableSamplers = nullptr;

					auto* bindingDescription = new VulkanDescriptorSetLayoutBindingDescription();
					bindingDescription->binding = layoutBinding;
					bindingDescription->set = setInfo->set;
					bindingDescription->name = binding->name;

					switch (binding->image.dim) {
						case SpvDim1D:
						{
							if (binding->image.arrayed) {
								bindingDescription->imageType = ImageType::sampler1D_ARRAY;
							} else {
								bindingDescription->imageType = ImageType::sampler1D;
							}
						}
							break;
						case SpvDim2D:
						{
							if (binding->image.arrayed) {
								bindingDescription->imageType = ImageType::sampler2D_ARRAY;
							} else {
								bindingDescription->imageType = ImageType::sampler2D;
							}
						}
							break;
						case SpvDim3D: 
							bindingDescription->imageType = ImageType::sampler3D;
							break;
						case SpvDimCube: 
						{
							if (binding->image.arrayed) {
								bindingDescription->imageType = ImageType::samplerCube_ARRAY;
							} else {
								bindingDescription->imageType = ImageType::samplerCube;
							}
						}
							break;
						case SpvDimRect:
						case SpvDimBuffer:
						case SpvDimSubpassData: 
							assert(false);
						default:
							break;
					}

					m_descriptorSetLayoutBindings[i].push_back(bindingDescription);
				}
					break;
				case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: // = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
				case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: // = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
				case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: // = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
				case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: // = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
					break;
				case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: // = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
				{
					const char* uniformTypeName = binding->type_description->type_name;
					const bool constUniform = strstr(uniformTypeName, "static");

					// создаем лаяут дескриптора юниформа или обновляем имеющийся
					VkDescriptorSetLayoutBinding layoutBinding;
					layoutBinding.binding = binding->binding;
					layoutBinding.descriptorType = constUniform ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
					layoutBinding.descriptorCount = binding->count;
					layoutBinding.stageFlags = m_stage;
					layoutBinding.pImmutableSamplers = nullptr;

					auto* bindingDescription = new VulkanDescriptorSetLayoutBindingDescription();
					bindingDescription->binding = layoutBinding;
					bindingDescription->set = setInfo->set;
					bindingDescription->name = binding->name;

					m_descriptorSetLayoutBindings[i].push_back(bindingDescription);

					uint32_t sizeInBytes = 0u;
					parseBuffer(binding, bindingDescription, sizeInBytes);

					const auto minUboAlignment = static_cast<uint32_t>(renderer->getDevice()->gpuProperties.limits.minUniformBufferOffsetAlignment);
					if (minUboAlignment > 0u) {
						sizeInBytes = engine::alignValue(sizeInBytes, minUboAlignment);
					}

					bindingDescription->sizeInBytes = sizeInBytes;
				}
					break;
				case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: // = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
				{
					// создаем лаяут дескриптора юниформа или обновляем имеющийся
					VkDescriptorSetLayoutBinding layoutBinding;
					layoutBinding.binding = binding->binding;
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
					layoutBinding.descriptorCount = binding->count;
					layoutBinding.stageFlags = m_stage;
					layoutBinding.pImmutableSamplers = nullptr;

					auto* bindingDescription = new VulkanDescriptorSetLayoutBindingDescription();
					bindingDescription->binding = layoutBinding;
					bindingDescription->set = setInfo->set;
					bindingDescription->name = binding->name;

					m_descriptorSetLayoutBindings[i].push_back(bindingDescription);

					uint32_t sizeInBytes = 0u;
					parseBuffer(binding, bindingDescription, sizeInBytes);

					const size_t minUboAlignment = renderer->getDevice()->gpuProperties.limits.minUniformBufferOffsetAlignment;
					if (minUboAlignment > 0u) {
						sizeInBytes = engine::alignValue(sizeInBytes, minUboAlignment);
					}

					bindingDescription->sizeInBytes = sizeInBytes;
				}
					break;
				case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: // = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
				{
					const char* uniformTypeName = binding->type_description->type_name;
					const bool constUniform = strstr(uniformTypeName, "static");

					// создаем лаяут дескриптора юниформа или обновляем имеющийся
					VkDescriptorSetLayoutBinding layoutBinding;
					layoutBinding.binding = binding->binding;
					layoutBinding.descriptorType = constUniform ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
					layoutBinding.descriptorCount = binding->count;
					layoutBinding.stageFlags = m_stage;
					layoutBinding.pImmutableSamplers = nullptr;

					auto* bindingDescription = new VulkanDescriptorSetLayoutBindingDescription();
					bindingDescription->binding = layoutBinding;
					bindingDescription->set = setInfo->set;
					bindingDescription->name = binding->name;

					m_descriptorSetLayoutBindings[i].push_back(bindingDescription);

					uint32_t sizeInBytes = 0u;
					parseBuffer(binding, bindingDescription, sizeInBytes);

					const auto minUboAlignment = static_cast<uint32_t>(renderer->getDevice()->gpuProperties.limits.minUniformBufferOffsetAlignment);
					if (minUboAlignment > 0u) {
						sizeInBytes = engine::alignValue(sizeInBytes, minUboAlignment);
					}

					bindingDescription->sizeInBytes = sizeInBytes;
				}
					break;
				case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: // = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
				{
					// создаем лаяут дескриптора юниформа или обновляем имеющийся
					VkDescriptorSetLayoutBinding layoutBinding;
					layoutBinding.binding = binding->binding;
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
					layoutBinding.descriptorCount = binding->count;
					layoutBinding.stageFlags = m_stage;
					layoutBinding.pImmutableSamplers = nullptr;

					auto* bindingDescription = new VulkanDescriptorSetLayoutBindingDescription();
					bindingDescription->binding = layoutBinding;
					bindingDescription->set = setInfo->set;
					bindingDescription->name = binding->name;

					m_descriptorSetLayoutBindings[i].push_back(bindingDescription);

					uint32_t sizeInBytes = 0u;
					parseBuffer(binding, bindingDescription, sizeInBytes);

					const size_t minUboAlignment = renderer->getDevice()->gpuProperties.limits.minUniformBufferOffsetAlignment;
					if (minUboAlignment > 0u) {
						sizeInBytes = engine::alignValue(sizeInBytes, minUboAlignment);
					}

					bindingDescription->sizeInBytes = sizeInBytes;
				}
					break;
				case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: // = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
				case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:  // = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
				default:
					break;
				}
			}
		}

		// push константы
		uint32_t pushConstantsCount = 0;
		spvReflectEnumeratePushConstantBlocks(&reflectedModule, &pushConstantsCount, nullptr);
		std::vector<SpvReflectBlockVariable*> pushConstants(pushConstantsCount);
		spvReflectEnumeratePushConstantBlocks(&reflectedModule, &pushConstantsCount, pushConstants.data());

		m_pushConstantsRange.resize(pushConstantsCount);

		size_t i = 0;
		for (const SpvReflectBlockVariable* pushConstant : pushConstants) {

			VkPushConstantRange pushConstantDecription;
			pushConstantDecription.stageFlags = m_stage;
			pushConstantDecription.offset = pushConstant->offset;
			pushConstantDecription.size = pushConstant->size;

			VulkanPushConstantRangeInfo rangeInfo;
			rangeInfo.name = pushConstant->name;
			rangeInfo.range = pushConstantDecription;

			rangeInfo.childInfos.resize(pushConstant->member_count);

			for (uint32_t j = 0; j < pushConstant->member_count; ++j) {
				SpvReflectBlockVariable* v = &(pushConstant->members[j]);
				rangeInfo.childInfos[j] = { v->name, v->offset, v->size };
			}

			m_pushConstantsRange[i] = rangeInfo;
			++i;
		}

		// destroy the reflection data when no longer required
		spvReflectDestroyShaderModule(&reflectedModule);
	}

	VulkanGpuProgram::VulkanGpuProgram(VulkanRenderer* renderer,
                                       std::vector<ShaderStageInfo>& stages, bool isParamsOwner) :
    m_id(engine::getUniqueId<VulkanGpuProgram>()), m_renderer(renderer) {
		using namespace engine;

		const size_t stagesSize = stages.size();
		m_shaderStages.resize(stagesSize);

		std::vector<VulkanShaderModule*> modules;
		modules.resize(stagesSize);

		size_t i = 0u;
		for (const ShaderStageInfo& stageInfo : stages) {
			auto&& module = Engine::getInstance().getModule<CacheManager>().load<VulkanShaderModule*>(
					std::string(stageInfo.modulePass),
					[](VulkanRenderer* renderer, const ShaderStageInfo& stageInfo) -> VulkanShaderModule* {
                            if (stageInfo.shaderCode) {
                                return new VulkanShaderModule(renderer, *stageInfo.shaderCode, stageInfo.pipelineStage);
                            } else {
                                return new VulkanShaderModule(renderer, stageInfo);
                            }
                        },
					renderer, stageInfo
				);

			m_maxSetNum = m_maxSetNum > module->m_maxSet ? m_maxSetNum : module->m_maxSet;
			m_pushConstantsCount += module->m_pushConstantsRange.size();

			m_shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			m_shaderStages[i].pNext = nullptr;
			m_shaderStages[i].flags = 0;
			m_shaderStages[i].stage = module->m_stage;
			m_shaderStages[i].module = module->m_module;
			m_shaderStages[i].pName = "main";
			m_shaderStages[i].pSpecializationInfo = stageInfo.specializationInfo;

			modules[i] = module;

			assert(m_shaderStages[i].module != VK_NULL_HANDLE);
			++i;
		}

		parseModules(modules);
        if (isParamsOwner) {
            assignParams();
        }
	}

	VulkanGpuProgram::~VulkanGpuProgram() {
		for (auto&& it = m_paramLayouts.begin(); it != m_paramLayouts.end(); ++it) {
			delete it->second;
		}
		m_paramLayouts.clear();
		m_paramLayoutsVec.clear();

		uint8_t i = 0u;
		for (VulkanDescriptorSet* descriptorSet : m_descriptorSets) {
			if ((m_externalDescriptors & (1u << i++)) == 0u) {
				m_renderer->freeDescriptorSets(descriptorSet);
				delete descriptorSet;
			}
		}
		m_descriptorSets.clear();

		m_pushConstantsRanges.clear();

		m_staticGPUBuffers.clear();
	}

	void VulkanGpuProgram::parseModules(const std::vector<VulkanShaderModule*>& modules) {

		std::vector<std::vector<VkDescriptorSetLayoutBinding*>> descriptorSetLayoutBindings;

		descriptorSetLayoutBindings.resize(m_maxSetNum + 1);
		m_pushConstantsRanges.resize(m_pushConstantsCount);

		auto&& parseParamLayoutChilds = [](uint8_t &paramId, GPUParamLayoutInfo* parentInfo, VulkanGpuProgram::params_type& paramLayouts, const std::vector<VulkanUniformInfo>& childInfos, const GPUParamLayoutType type) {
			for (const VulkanUniformInfo& childInfo : childInfos) {
				auto* info = new GPUParamLayoutInfo{ paramId++, parentInfo->set, childInfo.offset, childInfo.sizeInBytes, nullptr, nullptr, type, 0 };
				info->parentLayout = parentInfo;
				paramLayouts[childInfo.name] = info;
			}
		};

		std::array<VulkanDescriptorSetLayoutBindingDescription*, 8> found_sets{}; // расчитываем на 8 sets max
		uint8_t sets_map = 0u;
		uint8_t paramId = 0u;
		for (VulkanShaderModule* module : modules) {
			for (auto&& descriptionVec : module->m_descriptorSetLayoutBindings) {
				for (VulkanDescriptorSetLayoutBindingDescription* layoutDescription : descriptionVec) {

					if (sets_map & (1 << layoutDescription->set)) { 
						found_sets[layoutDescription->set]->binding.stageFlags |= module->m_stage; // если в нескольких шейдерах прописан uniform с одинаковым set - ом, тогда проставим stageFlags для использования этого uniform во всех этих шейдерах
						continue; // пропускаем, чтоб не попал несколько раз в общее descriptorSetLayoutBindings для программы
					} 

					sets_map |= 1 << (layoutDescription->set);
					found_sets[layoutDescription->set] = layoutDescription;

					GPUParamLayoutInfo* info = nullptr;

					switch (layoutDescription->binding.descriptorType) {
						case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
						{
							info = new GPUParamLayoutInfo{ paramId++, layoutDescription->set, 0, layoutDescription->sizeInBytes, &(layoutDescription->binding), nullptr, GPUParamLayoutType::UNIFORM_BUFFER };
							parseParamLayoutChilds(paramId, info, m_paramLayouts, layoutDescription->childInfos, GPUParamLayoutType::BUFFER_PART);
						}
							break;
						case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
						{
							info = new GPUParamLayoutInfo{ paramId++, layoutDescription->set, 0, layoutDescription->sizeInBytes, &(layoutDescription->binding), nullptr, GPUParamLayoutType::UNIFORM_BUFFER_DYNAMIC };
							info->dynamicBufferIdx = m_dynamicBuffersCount++;
							parseParamLayoutChilds(paramId, info, m_paramLayouts, layoutDescription->childInfos, GPUParamLayoutType::BUFFER_DYNAMIC_PART);
						}
							break;
						case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
						{
							info = new GPUParamLayoutInfo{ paramId++, layoutDescription->set, 0, layoutDescription->sizeInBytes, &(layoutDescription->binding), nullptr, GPUParamLayoutType::STORAGE_BUFFER };
							parseParamLayoutChilds(paramId, info, m_paramLayouts, layoutDescription->childInfos, GPUParamLayoutType::BUFFER_PART);
						}
							break;
						case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
						{
							info = new GPUParamLayoutInfo{ paramId++, layoutDescription->set, 0, layoutDescription->sizeInBytes, &(layoutDescription->binding), nullptr, GPUParamLayoutType::STORAGE_BUFFER_DYNAMIC };
							info->dynamicBufferIdx = m_dynamicBuffersCount++;
							parseParamLayoutChilds(paramId, info, m_paramLayouts, layoutDescription->childInfos, GPUParamLayoutType::BUFFER_DYNAMIC_PART);
						}
							break;
						case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
						{
							info = new GPUParamLayoutInfo{ paramId++, layoutDescription->set, 0, layoutDescription->sizeInBytes, &(layoutDescription->binding), nullptr, GPUParamLayoutType::COMBINED_IMAGE_SAMPLER };
							info->imageType = layoutDescription->imageType;
						}
							break;
						default:
							break;
					}

					descriptorSetLayoutBindings[layoutDescription->set].push_back(&(layoutDescription->binding));
					m_paramLayouts[layoutDescription->name] = info;
				}
			}

			size_t i = 0;
			for (VulkanPushConstantRangeInfo& ri : module->m_pushConstantsRange) {
				auto* info = new GPUParamLayoutInfo{ paramId++, 0, ri.range.offset, ri.range.size, nullptr, &ri.range, GPUParamLayoutType::PUSH_CONSTANT, static_cast<uint32_t>(i) };
				parseParamLayoutChilds(paramId, info, m_paramLayouts, ri.childInfos, GPUParamLayoutType::PUSH_CONSTANT_PART);
				m_pushConstantsRanges[i] = &ri.range;
				m_paramLayouts[ri.name] = info;
				++i;
			}
		}

		// create sorted layouts vector
		const size_t paramsCount = m_paramLayouts.size();
		m_paramLayoutsVec.reserve(paramsCount);

        for (auto&& [name, layout] : m_paramLayouts) {
            m_paramLayoutsVec.push_back(layout);
        }

		// сортировка m_paramLayoutsVec нужна для случая, когда в RenderData::prepareRender идет перебор layouts, 
		// чтобы для случаев полных буфферов(full buffer) не нужно было делать проверки вида if ((increasedBuffers & (uint64_t(1) << l->dynamcBufferIdx)) == 0),
		// проще тут один раз отсортировать
		std::sort(m_paramLayoutsVec.begin(), m_paramLayoutsVec.end(), [](const GPUParamLayoutInfo* p1, const GPUParamLayoutInfo* p2) {
				if (p1->type == p2->type) {
					return p1->set < p2->set; // упорядочивание дескрипторов по set id
				}
				
				return static_cast<uint8_t>(p1->type) < static_cast<uint8_t>(p2->type); // упорядочивание по типам
			}
		);

		////// generate program data
		m_pipelineDescriptorLayout = &m_renderer->getDescriptorLayout(descriptorSetLayoutBindings, m_pushConstantsRanges);

#ifdef  GPU_DEBUG_MARKERS

		GPU_DEBUG_MARKER_SET_OBJECT_NAME(m_pipelineDescriptorLayout->pipelineLayout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, engine::fmt_string("j4f pipeline layout (program %d)", m_id));

		uint8_t set = 0;
		for (VkDescriptorSetLayout& descriptorSetLayout : m_pipelineDescriptorLayout->descriptorSetLayouts) {
			GPU_DEBUG_MARKER_SET_OBJECT_NAME(descriptorSetLayout, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, engine::fmt_string("j4f descriptor set layout (set %d, program %d)", set, m_id));
			++set;
		}
#endif //  GPU_DEBUG_MARKERS

		
		// allocate descriptor sets
		if (!m_pipelineDescriptorLayout->descriptorSetLayouts.empty()) {

			uint8_t buffersCount = 0;
			uint8_t staticBuffersCount = 0;
			uint8_t descriptorsCount = 0;
			m_gpuBuffersSetsTypes = 0;
			for (auto& descriptorSetLayoutBinding : descriptorSetLayoutBindings) {
				for (VkDescriptorSetLayoutBinding* binding : descriptorSetLayoutBinding) {
					switch (binding->descriptorType) {
						case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
						case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
							++buffersCount;
							++staticBuffersCount;
							++descriptorsCount;
							break;
						case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
						case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
							m_gpuBuffersSetsTypes |= (1 << descriptorsCount);
							++buffersCount;
							++descriptorsCount;
							break;
						case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
							m_externalDescriptors |= (1 << descriptorsCount);
							++descriptorsCount;
							break;
						default:
							break;
					}
				}
			}

			for (uint8_t i = 0u; i < descriptorsCount; ++i) {
				if (m_externalDescriptors & (1u << i)) {
					m_descriptorSets.push_back(nullptr);
				} else {
					VulkanDescriptorSet* newDescriptorSet = m_renderer->allocateDescriptorSetFromGlobalPool(m_pipelineDescriptorLayout->descriptorSetLayouts[i], (m_gpuBuffersSetsTypes & (1 << i)) ? 0 : 1);
					m_descriptorSets.push_back(newDescriptorSet);
				}
			}

			m_staticGPUBuffers.resize(staticBuffersCount);
		}
	}

	void VulkanGpuProgram::assignParams() {
		uint32_t pushConstantNum = 0u;
		uint32_t staticUBONum = 0u;
		for (auto&& it = m_paramLayouts.begin(); it != m_paramLayouts.end(); ++it) {
			GPUParamLayoutInfo* layout = it->second;
			if (layout->sizeInBytes == 0u) continue;

			switch (layout->type) {
				case GPUParamLayoutType::UNIFORM_BUFFER:
				{
					VulkanDescriptorSet* descriptorSet = m_descriptorSets[layout->set];
					m_renderer->getDevice()->createBuffer(
						VK_SHARING_MODE_EXCLUSIVE,
						VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
						&m_staticGPUBuffers[staticUBONum],
						layout->sizeInBytes
					);

					m_renderer->bindBufferToDescriptorSet(
						descriptorSet,
						VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
						&m_staticGPUBuffers[staticUBONum],
						layout->descriptorSetLayoutBinding->binding,
						layout->sizeInBytes,
						0u
					);

					layout->data = &m_staticGPUBuffers[staticUBONum];
					m_gpuBuffersSets |= (1 << layout->set);
					++staticUBONum;
				}
					break;
				case GPUParamLayoutType::STORAGE_BUFFER:
				{
					VulkanDescriptorSet* descriptorSet = m_descriptorSets[layout->set];
					m_renderer->getDevice()->createBuffer(
						VK_SHARING_MODE_EXCLUSIVE,
						VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
						&m_staticGPUBuffers[staticUBONum],
						layout->sizeInBytes
					);

					m_renderer->bindBufferToDescriptorSet(
						descriptorSet,
						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						&m_staticGPUBuffers[staticUBONum],
						layout->descriptorSetLayoutBinding->binding,
						layout->sizeInBytes,
						0u
					);

					layout->data = &m_staticGPUBuffers[staticUBONum];
					m_gpuBuffersSets |= (1 << layout->set);
					++staticUBONum;
				}
					break;
				case GPUParamLayoutType::UNIFORM_BUFFER_DYNAMIC:
				{
					m_dynamicGPUBuffers.push_back(m_renderer->getDynamicGPUBufferForSize(layout->sizeInBytes, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));
					m_renderer->bindDynamicUniformBufferToDescriptorSet(m_descriptorSets[layout->set], m_dynamicGPUBuffers.back(), layout->descriptorSetLayoutBinding->binding);
					layout->data = m_dynamicGPUBuffers.back();
					m_gpuBuffersSets |= (1 << layout->set);
				}
					break;
				case GPUParamLayoutType::STORAGE_BUFFER_DYNAMIC:
				{
					m_dynamicGPUBuffers.push_back(m_renderer->getDynamicGPUBufferForSize(layout->sizeInBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));
					m_renderer->bindDynamicStorageBufferToDescriptorSet(m_descriptorSets[layout->set], m_dynamicGPUBuffers.back(), layout->descriptorSetLayoutBinding->binding);
					layout->data = m_dynamicGPUBuffers.back();
					m_gpuBuffersSets |= (1 << layout->set);
				}
					break;
				case GPUParamLayoutType::PUSH_CONSTANT:
				{
					layout->data = m_pushConstantsRanges[pushConstantNum++];
				}
					break;
				case GPUParamLayoutType::COMBINED_IMAGE_SAMPLER:
				{
				}
					break;
				default:
					break;
			}
		}

		for (int16_t i = 0; i < m_maxSetNum + 1; ++i) { // 8 сетов максимум, хватит?
			if (m_gpuBuffersSets & (1 << i)) ++m_gpuBuffersSetsCount;
		}
	}

	uint32_t VulkanGpuProgram::setValueToLayout(const GPUParamLayoutInfo* paramLayout, const void* value, VulkanPushConstant* pConstant, const uint32_t knownOffset, const uint32_t knownSize, const bool allBuffers) {
		uint32_t result = UNDEFINED;

		switch (paramLayout->type) {
			case GPUParamLayoutType::UNIFORM_BUFFER_DYNAMIC: // full buffer
			{
				auto* buffer = reinterpret_cast<VulkanDynamicBuffer*>(paramLayout->data);
				result = m_renderer->updateDynamicBufferData(buffer, value, allBuffers, knownOffset, knownSize);
			}
				break;
			case GPUParamLayoutType::STORAGE_BUFFER_DYNAMIC: // full buffer
			{
                auto* buffer = reinterpret_cast<VulkanDynamicBuffer*>(paramLayout->data);
				result = m_renderer->updateDynamicBufferData(buffer, value, allBuffers, knownOffset, knownSize);
			}
				break;
			case GPUParamLayoutType::BUFFER_DYNAMIC_PART: // part of dynamic buffer
			{
                auto* buffer = reinterpret_cast<VulkanDynamicBuffer*>(paramLayout->parentLayout->data);
				result = m_renderer->updateDynamicBufferData(buffer, value, paramLayout->offset, (knownSize == UNDEFINED ? paramLayout->sizeInBytes : knownSize), allBuffers, knownOffset);
			}
				break;
			case GPUParamLayoutType::PUSH_CONSTANT: // full constant
			{
                auto* constant = reinterpret_cast<VkPushConstantRange*>(paramLayout->data);
				pConstant->range = constant;
				memcpy(pConstant->values, value, paramLayout->sizeInBytes);
			}
				break;
			case GPUParamLayoutType::PUSH_CONSTANT_PART: // part of constant
			{
                auto* constant = reinterpret_cast<VkPushConstantRange*>(paramLayout->parentLayout->data);
				pConstant->range = constant;
				memcpy(reinterpret_cast<void*>(reinterpret_cast<size_t>(pConstant->values) + paramLayout->offset), value, paramLayout->sizeInBytes);
			}
				break;
			case GPUParamLayoutType::COMBINED_IMAGE_SAMPLER:
			{
			}
				break;
			case GPUParamLayoutType::UNIFORM_BUFFER: // full buffer
			{
                auto* buffer = reinterpret_cast<vulkan::VulkanBuffer*>(paramLayout->data);
				void* memory = buffer->map(buffer->m_size);
				memcpy(reinterpret_cast<void*>(reinterpret_cast<size_t>(memory) + (knownOffset == UNDEFINED ? paramLayout->offset : knownOffset)), value, (knownSize == UNDEFINED ? paramLayout->sizeInBytes : knownSize));
				buffer->unmap();
			}
				break;
			case GPUParamLayoutType::STORAGE_BUFFER: // full buffer
			{
                auto* buffer = reinterpret_cast<vulkan::VulkanBuffer*>(paramLayout->data);
				void* memory = buffer->map(buffer->m_size);
				memcpy(reinterpret_cast<void*>(reinterpret_cast<size_t>(memory) + (knownOffset == UNDEFINED ? paramLayout->offset : knownOffset)), value, (knownSize == UNDEFINED ? paramLayout->sizeInBytes : knownSize));
				buffer->unmap();
			}
				break;
			case GPUParamLayoutType::BUFFER_PART: // part of buffer
			{
                auto* buffer = reinterpret_cast<vulkan::VulkanBuffer*>(paramLayout->parentLayout->data);
				void* memory = buffer->map(buffer->m_size);
				memcpy(reinterpret_cast<void*>(reinterpret_cast<size_t>(memory) + (knownOffset == UNDEFINED ? paramLayout->offset : knownOffset)), value, (knownSize == UNDEFINED ? paramLayout->sizeInBytes : knownSize));
				buffer->unmap();
			}
				break;
			default:
				break;
		}

		return result;
	}

	void VulkanGpuProgram::finishUpdateParams() {
		for (VulkanDynamicBuffer* ub : m_dynamicGPUBuffers) {
			ub->encrease();
		}
	}
}