#pragma once

#include "vkDescriptorSet.h"
#include "vkPipeline.h"
#include "vkBuffer.h"
#include "vkGPUProgram.h"
#include "vkCommandBuffer.h"
#include "vkTexture.h"
#include "vkRenderer.h"
#include "../Render/GpuProgramParam.h"

#include "../../Core/Engine.h"
#include "../../Core/Handler.h"
#include "../Graphics.h"

#include <cstdint>
#include <algorithm>
#include <map>
#include <memory>

namespace vulkan {

	using RenderDataGpuParamsType = std::shared_ptr<engine::GpuProgramParams>;
	//using RenderDataGpuParamsType = engine::handler_ptr<engine::GpuProgramParams>;
	//using RenderDataGpuParamsType = engine::GpuProgramParams;
	//using RenderDataGpuParamsType = engine::GpuProgramParams*;

	struct RenderData {
		struct RenderPart {
			uint32_t firstIndex;					// номер первого индекса
			uint32_t indexCount;					// количество индексов
			uint32_t vertexCount;					// количество вершин
			uint32_t firstVertex = 0;				// номер первой вершины
			uint32_t instanceCount = 1;				// количество инстансов
			uint32_t firstInstance = 0;				// номер первого инстанса
			VkDeviceSize vbOffset = 0;				// оффсет в вершинном буфере
			VkDeviceSize ibOffset = 0;				// оффсет в индексном буфере
		};

		std::vector<std::pair<GPUParamLayoutInfo*, uint32_t>> layouts;
		std::vector<uint32_t> dynamicOffsets;
		std::vector<vulkan::VulkanPushConstant> constants;
		RenderDataGpuParamsType params;

		vulkan::VulkanPipeline* pipeline;

		const VulkanBuffer* vertexes = nullptr;		// вершины
		const VulkanBuffer* indexes = nullptr;		// индексы

		RenderPart* renderParts;
		uint8_t renderPartsCount = 0;
		bool visible = true;

		RenderData() : pipeline(nullptr), renderParts(nullptr), params(std::make_shared<engine::GpuProgramParams>()) {
		}

		RenderData(VulkanPipeline* p) : pipeline(p), renderParts(nullptr), params(std::make_shared<engine::GpuProgramParams>()) {
			prepareLayouts();
		}

		RenderData(VulkanPipeline* p, const RenderDataGpuParamsType& gpu_params) : pipeline(p), renderParts(nullptr), params(gpu_params) {
			prepareLayouts();
		}

		~RenderData() {
			pipeline = VK_NULL_HANDLE;
			resetGpuData();
			deleteRenderParts();
		}

		inline void setPipeline(VulkanPipeline* p) {
			if (pipeline == nullptr) {
				pipeline = p;
				prepareLayouts();
			} else if (pipeline->program != p->program) {
				pipeline = p;
				resetGpuData();
				prepareLayouts();
			} else {
				pipeline = p;
			}
		}

		inline void resetGpuData() {
			layouts.clear();
			dynamicOffsets.clear();
			constants.clear();
		}

		inline void replaceParams(RenderDataGpuParamsType& p) { params = p; }

		inline void createRenderParts(const uint8_t size) {
			renderPartsCount = size;
			renderParts = new vulkan::RenderData::RenderPart[renderPartsCount];
		}

		inline void deleteRenderParts() {
			if (renderParts) {
				delete[] renderParts;
				renderParts = nullptr;
				renderPartsCount = 0;
			}
		}

		inline void setRenderParts(vulkan::RenderData::RenderPart* parts, const uint8_t size) {
			renderPartsCount = size;
			renderParts = parts;
		}

		inline void replaceRenderParts(vulkan::RenderData::RenderPart* parts, const uint8_t size) {
			deleteRenderParts();
			renderPartsCount = size;
			renderParts = parts;
		}

		inline const GPUParamLayoutInfo* getLayout(const std::string& name) const {
			return pipeline->program->getGPUParamLayoutByName(name);
		}

		inline void setRawDataForLayout(const GPUParamLayoutInfo* info, void* value, const bool copyData, const size_t valueSize) {
			params->operator[](info->id).setRawData(value, valueSize, copyData);
		}

		inline bool setRawDataByName(const std::string& name, void* value, const bool copyData, const size_t valueSize) {
			if (auto&& layout = getLayout(name)) {
				setRawDataForLayout(layout, value, copyData, valueSize);
				return true;
			}
			return false;
		}

		template <typename T>
		inline void setParamForLayout(const GPUParamLayoutInfo* info, T* value, const bool copyData, const uint32_t count = 1) {
			params->operator[](info->id).setValue(value, count, copyData);
		}

		template <typename T>
		inline void setParamForLayout(const GPUParamLayoutInfo* info, T&& value, const bool copyData, const uint32_t count = 1) {
			params->operator[](info->id).setValue(&value, count, copyData);
		}

		template <typename T>
		inline bool setParamByName(const std::string& name, T* value, bool copyData, const uint32_t count = 1) {
			if (auto&& layout = getLayout(name)) {
				setParamForLayout(layout, value, copyData, count);
				return true;
			}
			return false;
		}

		template <typename T>
		inline bool setParamByName(const std::string& name, T&& value, bool copyData, const uint32_t count = 1) {
			if (auto&& layout = getLayout(name)) {
				setParamForLayout(layout, value, copyData, count);
				return true;
			}
			return false;
		}

		void prepareLayouts() {
			if (pipeline == nullptr) return;

			using namespace vulkan;

			const VulkanGpuProgram* program = pipeline->program;
			const std::vector<GPUParamLayoutInfo*>& programParamLayouts = program->getParamsLayoutSortedVec();
			const size_t paramsCount = programParamLayouts.size();
			if (params->size() < paramsCount) {
				params->resize(paramsCount);
			}

			layouts.reserve(paramsCount);

			uint16_t dynamicOffsetsCount = 0;
			uint16_t constantsCount = 0;

			for (GPUParamLayoutInfo* l : programParamLayouts) {
				auto& pair = layouts.emplace_back(l, 0);
				switch (l->type) {
					case GPUParamLayoutType::UNIFORM_BUFFER_DYNAMIC: // full buffer
					{
						++dynamicOffsetsCount;
					}
						break;
					case GPUParamLayoutType::STORAGE_BUFFER_DYNAMIC: // full buffer
					{
						++dynamicOffsetsCount;
					}
						break;
					case GPUParamLayoutType::BUFFER_DYNAMIC_PART: // dynamic buffer part
					{
					}
						break;
					case GPUParamLayoutType::PUSH_CONSTANT: // full constant
					{
						++constantsCount;
					}
						break;
					case GPUParamLayoutType::PUSH_CONSTANT_PART: // part of constant
					{
					}
						break;
					default:
						break;
				}
			}

			dynamicOffsets.resize(dynamicOffsetsCount);
			constants.resize(constantsCount);
		}

		void prepareRender(VulkanCommandBuffer& commandBuffer) {
			using namespace vulkan;

			VulkanGpuProgram* program = const_cast<VulkanGpuProgram*>(pipeline->program);

			for (auto&& p : layouts) {
				const GPUParamLayoutInfo* l = p.first;
				const void* value = params->operator[](l->id).getValue();
				const size_t size = params->operator[](l->id).size;
				switch (l->type) {
					case GPUParamLayoutType::UNIFORM_BUFFER_DYNAMIC: // full buffer
					{
						VulkanDynamicBuffer* buffer = reinterpret_cast<VulkanDynamicBuffer*>(l->data);
						p.second = buffer->encrease();
						if (value) {
							dynamicOffsets[l->descriptorSetLayoutBinding->binding] = program->setValueToLayout(l, value, nullptr, p.second, size);
						}
					}
						break;
					case GPUParamLayoutType::STORAGE_BUFFER_DYNAMIC: // full buffer
					{
						VulkanDynamicBuffer* buffer = reinterpret_cast<VulkanDynamicBuffer*>(l->data);
						p.second = buffer->encrease();
						if (value) {
							dynamicOffsets[l->descriptorSetLayoutBinding->binding] = program->setValueToLayout(l, value, nullptr, p.second, size);
						}
					}
						break;
					case GPUParamLayoutType::BUFFER_DYNAMIC_PART: // dynamic buffer part
					{
						if (value) {
							VulkanDynamicBuffer* buffer = reinterpret_cast<VulkanDynamicBuffer*>(l->parentLayout->data);
							p.second = buffer->getCurrentOffset() - 1; // -1 because buffer offset has been encreased

							dynamicOffsets[l->parentLayout->descriptorSetLayoutBinding->binding] = program->setValueToLayout(l, value, nullptr, p.second, size);
						}
					}
						break;
					case GPUParamLayoutType::UNIFORM_BUFFER:
					{
						if (value) {
							program->setValueToLayout(l, value, nullptr);
						}
					}
						break;
					case GPUParamLayoutType::STORAGE_BUFFER:
					{
						if (value) {
							program->setValueToLayout(l, value, nullptr);
						}
					}
						break;
					case GPUParamLayoutType::BUFFER_PART:
					{
						if (value) {
							program->setValueToLayout(l, value, nullptr);
						}
					}
						break;
					case GPUParamLayoutType::PUSH_CONSTANT: // full constant
					{
						if (value) {
							program->setValueToLayout(l, value, &constants[l->push_constant_number], 0);
						}
					}
						break;
					case GPUParamLayoutType::PUSH_CONSTANT_PART: // part of constant
					{
						if (value) {
							program->setValueToLayout(l, value, &constants[l->parentLayout->push_constant_number], 0);
						}
					}
						break;
					case GPUParamLayoutType::COMBINED_IMAGE_SAMPLER: // image sampler
					{
						if (value) { // bind single texture set only if it change previous
							const VulkanTexture* texture = static_cast<const VulkanTexture*>(value);
							const VkDescriptorSet set = texture->getSingleDescriptor();
							if (set != VK_NULL_HANDLE) {
								commandBuffer.cmdBindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, program->getPipeLineLayout(), l->set, set);
							} else {
								const VkDescriptorSet emptySet = engine::Engine::getInstance().getModule<engine::Graphics>()->getRenderer()->getEmptyTexture()->getSingleDescriptor();
								commandBuffer.cmdBindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, program->getPipeLineLayout(), l->set, emptySet);
							}
						} else {
							const VkDescriptorSet emptySet = engine::Engine::getInstance().getModule<engine::Graphics>()->getRenderer()->getEmptyTexture()->getSingleDescriptor();
							commandBuffer.cmdBindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, program->getPipeLineLayout(), l->set, emptySet);
						}
					}
						break;
					default:
						break;
				}
			}
		}

		inline void render(VulkanCommandBuffer& commandBuffer, const uint32_t frame, const uint32_t adittionalSetsCount, const VkDescriptorSet* adittionalSets) const {
			if (indexes) {
				for (uint8_t i = 0; i < renderPartsCount; ++i) {
					const RenderPart& part = renderParts[i];
					commandBuffer.renderIndexed(
						pipeline, frame, constants.data(),
						0, dynamicOffsets.size(), dynamicOffsets.data(),
						adittionalSetsCount, adittionalSets,
						*vertexes, *indexes, part.firstIndex, part.indexCount, part.firstVertex, part.instanceCount, part.firstInstance, part.vbOffset, part.ibOffset
					);
				}
			} else {
				for (uint8_t i = 0; i < renderPartsCount; ++i) {
					const RenderPart& part = renderParts[i];
					commandBuffer.render(
						pipeline, frame, constants.data(),
						0, dynamicOffsets.size(), dynamicOffsets.data(),
						adittionalSetsCount, adittionalSets,
						*vertexes, part.firstVertex, part.vertexCount, part.instanceCount, part.firstInstance
					);
				}
			}
		}
	};

}