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

		std::vector<VkDescriptorSet> externalDescriptorsSets;

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
			uint16_t externalDescriptorsSetsCount = 0;

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
					case GPUParamLayoutType::COMBINED_IMAGE_SAMPLER: // texture + sampler
					{
						++externalDescriptorsSetsCount;
					}
						break;
					default:
						break;
				}
			}

			dynamicOffsets.resize(dynamicOffsetsCount);
			constants.resize(constantsCount);
			externalDescriptorsSets.resize(externalDescriptorsSetsCount);
		}

		void prepareRender(VulkanCommandBuffer& commandBuffer) {
			using namespace vulkan;

			VulkanGpuProgram* program = const_cast<VulkanGpuProgram*>(pipeline->program);
			uint32_t increasedBuffers = 0;
			uint8_t externalSetNum = 0;

			for (auto&& p : layouts) {
				const GPUParamLayoutInfo* l = p.first;
				const void* value = params->operator[](l->id).getValue();
				const size_t size = params->operator[](l->id).size;
				switch (l->type) {
					case GPUParamLayoutType::UNIFORM_BUFFER_DYNAMIC: // full buffer
					{
						VulkanDynamicBuffer* buffer = reinterpret_cast<VulkanDynamicBuffer*>(l->data);

						if (value) {
							p.second = buffer->encrease();
							increasedBuffers |= (uint64_t(1) << l->dynamcBufferIdx);
							dynamicOffsets[l->dynamcBufferIdx] = program->setValueToLayout(l, value, nullptr, p.second, size);
						} else {
							const uint32_t bufferOffset = buffer->getCurrentOffset();
							dynamicOffsets[l->dynamcBufferIdx] = bufferOffset == 0 ? 0 : (buffer->alignedSize * (bufferOffset - 1));
						}
					}
						break;
					case GPUParamLayoutType::STORAGE_BUFFER_DYNAMIC: // full buffer
					{
						VulkanDynamicBuffer* buffer = reinterpret_cast<VulkanDynamicBuffer*>(l->data);

						if (value) {
							p.second = buffer->encrease();
							increasedBuffers |= (uint64_t(1) << l->dynamcBufferIdx);
							dynamicOffsets[l->dynamcBufferIdx] = program->setValueToLayout(l, value, nullptr, p.second, size);
						} else {
							const uint32_t bufferOffset = buffer->getCurrentOffset();
							dynamicOffsets[l->dynamcBufferIdx] = bufferOffset == 0 ? 0 : (buffer->alignedSize * (bufferOffset - 1));
						}
					}
						break;
					case GPUParamLayoutType::BUFFER_DYNAMIC_PART: // dynamic buffer part
					{
						if (value) {
							VulkanDynamicBuffer* buffer = reinterpret_cast<VulkanDynamicBuffer*>(l->parentLayout->data);
							if ((increasedBuffers & (uint64_t(1) << l->parentLayout->dynamcBufferIdx)) == 0) {
								increasedBuffers |= (uint64_t(1) << l->parentLayout->dynamcBufferIdx);
								p.second = buffer->encrease();
							} else {
								const uint32_t bufferOffset = buffer->getCurrentOffset();
								p.second = bufferOffset == 0 ? 0 : bufferOffset - 1;
							}

							dynamicOffsets[l->parentLayout->dynamcBufferIdx] = program->setValueToLayout(l, value, nullptr, p.second, size);
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
								//commandBuffer.cmdBindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, program->getPipeLineLayout(), l->set, set);
								externalDescriptorsSets[externalSetNum++] = set;
							} else {
								switch (l->imageType) {
									case GPUParamLayoutInfo::ImageType::sampler2D_ARRAY:
									{
										const VkDescriptorSet emptySet = engine::Engine::getInstance().getModule<engine::Graphics>()->getRenderer()->getEmptyTextureArray()->getSingleDescriptor();
										externalDescriptorsSets[externalSetNum++] = emptySet;
									}
										break;
									case GPUParamLayoutInfo::ImageType::sampler2D:
									default:
									{
										const VkDescriptorSet emptySet = engine::Engine::getInstance().getModule<engine::Graphics>()->getRenderer()->getEmptyTexture()->getSingleDescriptor();
										externalDescriptorsSets[externalSetNum++] = emptySet;
									}
										break;
								}
								//const VkDescriptorSet emptySet = engine::Engine::getInstance().getModule<engine::Graphics>()->getRenderer()->getEmptyTexture()->getSingleDescriptor();
								////commandBuffer.cmdBindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, program->getPipeLineLayout(), l->set, emptySet);
								//externalDescriptorsSets[externalSetNum++] = emptySet;
							}
						} else {
							switch (l->imageType) {
								case GPUParamLayoutInfo::ImageType::sampler2D_ARRAY:
								{
									const VkDescriptorSet emptySet = engine::Engine::getInstance().getModule<engine::Graphics>()->getRenderer()->getEmptyTextureArray()->getSingleDescriptor();
									externalDescriptorsSets[externalSetNum++] = emptySet;
								}
									break;
								case GPUParamLayoutInfo::ImageType::sampler2D:
								default:
								{
									const VkDescriptorSet emptySet = engine::Engine::getInstance().getModule<engine::Graphics>()->getRenderer()->getEmptyTexture()->getSingleDescriptor();
									externalDescriptorsSets[externalSetNum++] = emptySet;
								}
									break;
							}
							//const VkDescriptorSet emptySet = engine::Engine::getInstance().getModule<engine::Graphics>()->getRenderer()->getEmptyTexture()->getSingleDescriptor();
							////commandBuffer.cmdBindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, program->getPipeLineLayout(), l->set, emptySet);
							//externalDescriptorsSets[externalSetNum++] = emptySet;
						}
					}
						break;
					default:
						break;
				}
			}
		}

		inline void render(VulkanCommandBuffer& commandBuffer, const uint32_t frame) const {
			if (indexes) {
				for (uint8_t i = 0; i < renderPartsCount; ++i) {
					const RenderPart& part = renderParts[i];
					commandBuffer.renderIndexed(
						pipeline, frame, constants.data(),
						0, dynamicOffsets.size(), dynamicOffsets.data(),
						externalDescriptorsSets.size(), externalDescriptorsSets.data(),
						*vertexes, *indexes, part.firstIndex, part.indexCount, part.firstVertex, part.instanceCount, part.firstInstance, part.vbOffset, part.ibOffset
					);
				}
			} else {
				for (uint8_t i = 0; i < renderPartsCount; ++i) {
					const RenderPart& part = renderParts[i];
					commandBuffer.render(
						pipeline, frame, constants.data(),
						0, dynamicOffsets.size(), dynamicOffsets.data(),
						externalDescriptorsSets.size(), externalDescriptorsSets.data(),
						*vertexes, part.firstVertex, part.vertexCount, part.instanceCount, part.firstInstance
					);
				}
			}
		}
	};

}