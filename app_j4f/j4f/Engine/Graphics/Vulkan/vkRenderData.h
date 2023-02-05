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

    struct BatchingParams {
        uint32_t vtxDataSize = 0;
        uint32_t idxDataSize = 0;
        const void* rawVertexes = nullptr; // raw data for autobatching mechanism
        const void* rawIndexes = nullptr;
    };

	struct RenderData {
		struct RenderPart {
			uint32_t firstIndex = 0;				// номер первого индекса
			uint32_t indexCount = 0;				// количество индексов
			uint32_t vertexCount = 0;				// количество вершин
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

        size_t vertexSize = 0;
        BatchingParams* batchingParams = nullptr;
        VkIndexType indexType = VK_INDEX_TYPE_UINT32;

		std::vector<VkDescriptorSet> externalDescriptorsSets;

		RenderPart* renderParts = nullptr;
		uint8_t renderPartsCount = 0;
		bool visible = true;

		RenderData() : pipeline(nullptr), renderParts(nullptr), params(std::make_shared<engine::GpuProgramParams>()) { }

		explicit RenderData(VulkanPipeline* p) : pipeline(p), renderParts(nullptr), params(std::make_shared<engine::GpuProgramParams>()) {
			prepareLayouts();
		}

        explicit RenderData(const RenderDataGpuParamsType& gpu_params) : pipeline(nullptr), renderParts(nullptr), params(gpu_params) { }
        explicit RenderData(RenderDataGpuParamsType&& gpu_params) : pipeline(nullptr), renderParts(nullptr), params(std::move(gpu_params)) { }

		RenderData(VulkanPipeline* p, const RenderDataGpuParamsType& gpu_params) : pipeline(p), renderParts(nullptr), params(gpu_params) {
			prepareLayouts();
		}

        RenderData(VulkanPipeline* p, RenderDataGpuParamsType&& gpu_params) : pipeline(p), renderParts(nullptr), params(std::move(gpu_params)) {
            prepareLayouts();
        }

		~RenderData() {
			pipeline = VK_NULL_HANDLE;
			resetGpuData();
			deleteRenderParts();

            if (batchingParams) {
                delete batchingParams;
                batchingParams = nullptr;
            }
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

		inline void replaceParams(const RenderDataGpuParamsType& p) { params = p; }

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

		[[nodiscard]] inline const GPUParamLayoutInfo* getLayout(const std::string& name) const {
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
			const std::vector<GPUParamLayoutInfo*>& programParamLayouts = program->getParamsLayoutVec();
			const size_t paramsCount = programParamLayouts.size();
			if (params->size() < paramsCount) {
				params->resize(paramsCount);
			}

			layouts.reserve(paramsCount);

			uint16_t dynamicOffsetsCount = 0;
			uint16_t constantsCount = 0;
			uint16_t externalDescriptorsSetsCount = 0;

			for (GPUParamLayoutInfo* l : programParamLayouts) {
				//auto& pair = layouts.emplace_back(l, 0);
                layouts.emplace_back(l, 0);
				switch (l->type) {
                    case GPUParamLayoutType::UNIFORM_BUFFER_DYNAMIC: // full buffer
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

		uint32_t prepareRender(/*VulkanCommandBuffer& commandBuffer*/) {
			using namespace vulkan;

			auto* program = const_cast<VulkanGpuProgram*>(pipeline->program);
			uint32_t increasedBuffers = 0;
			uint8_t externalSetNum = 0;
			uint32_t result = 0;

			for (auto&& p : layouts) {
				const GPUParamLayoutInfo* l = p.first;
				const void* value = params->operator[](l->id).getValue();
				const size_t size = params->operator[](l->id).size;
				switch (l->type) {
					case GPUParamLayoutType::UNIFORM_BUFFER_DYNAMIC: // full buffer
					case GPUParamLayoutType::STORAGE_BUFFER_DYNAMIC: // full buffer
					{
						auto* buffer = reinterpret_cast<VulkanDynamicBuffer*>(l->data);

						if (value) {
							p.second = buffer->encrease();
							increasedBuffers |= (uint64_t(1) << l->dynamcBufferIdx);
							result = dynamicOffsets[l->dynamcBufferIdx] = program->setValueToLayout(l, value, nullptr, p.second, size);
						} else {
							const uint32_t bufferOffset = buffer->getCurrentOffset();
							result = dynamicOffsets[l->dynamcBufferIdx] = ((bufferOffset == 0) ? 0 : (buffer->alignedSize * (bufferOffset - 1)));
						}
					}
						break;
					case GPUParamLayoutType::BUFFER_DYNAMIC_PART: // dynamic buffer part
					{
						if (value) {
							auto* buffer = reinterpret_cast<VulkanDynamicBuffer*>(l->parentLayout->data);
							if ((increasedBuffers & (uint64_t(1) << l->parentLayout->dynamcBufferIdx)) == 0) {
								increasedBuffers |= (uint64_t(1) << l->parentLayout->dynamcBufferIdx);
								p.second = buffer->encrease();
							} else {
								const uint32_t bufferOffset = buffer->getCurrentOffset();
								p.second = bufferOffset == 0 ? 0 : bufferOffset - 1;
							}

							result = dynamicOffsets[l->parentLayout->dynamcBufferIdx] = program->setValueToLayout(l, value, nullptr, p.second, size);
						}
					}
						break;
					case GPUParamLayoutType::UNIFORM_BUFFER:
					case GPUParamLayoutType::STORAGE_BUFFER:
					case GPUParamLayoutType::BUFFER_PART:
					{
						if (value) {
							result = program->setValueToLayout(l, value, nullptr);
						}
					}
						break;
					case GPUParamLayoutType::PUSH_CONSTANT: // full constant
					{
						if (value) {
							result = program->setValueToLayout(l, value, &constants[l->push_constant_number], 0);
						}
					}
						break;
					case GPUParamLayoutType::PUSH_CONSTANT_PART: // part of constant
					{
						if (value) {
							result = program->setValueToLayout(l, value, &constants[l->parentLayout->push_constant_number], 0);
						}
					}
						break;
					case GPUParamLayoutType::COMBINED_IMAGE_SAMPLER: // image sampler
					{
						if (value) {
							const auto* texture = static_cast<const VulkanTexture*>(value);
                            VkDescriptorSet set = texture->getSingleDescriptor();

							if (set != VK_NULL_HANDLE) {
								externalDescriptorsSets[externalSetNum++] = set;
							} else {
								switch (l->imageType) {
									case ImageType::sampler2D_ARRAY:
										externalDescriptorsSets[externalSetNum++] = engine::Engine::getInstance().getModule<engine::Graphics>()->getRenderer()->getEmptyTextureArray()->getSingleDescriptor();
										break;
									case ImageType::sampler2D:
									default:
										externalDescriptorsSets[externalSetNum++] = engine::Engine::getInstance().getModule<engine::Graphics>()->getRenderer()->getEmptyTexture()->getSingleDescriptor();
										break;
								}
							}
						} else {
							switch (l->imageType) {
								case ImageType::sampler2D_ARRAY:
									externalDescriptorsSets[externalSetNum++] = engine::Engine::getInstance().getModule<engine::Graphics>()->getRenderer()->getEmptyTextureArray()->getSingleDescriptor();
									break;
								case ImageType::sampler2D:
								default:
									externalDescriptorsSets[externalSetNum++] = engine::Engine::getInstance().getModule<engine::Graphics>()->getRenderer()->getEmptyTexture()->getSingleDescriptor();
									break;
							}
						}
					}
						break;
					default:
						break;
				}
			}

			return result;
		}

		inline void render(VulkanCommandBuffer& commandBuffer, const uint32_t frame) const {
			if (indexes) {
				for (uint8_t i = 0; i < renderPartsCount; ++i) {
					const RenderPart& part = renderParts[i];
					commandBuffer.renderIndexed(
						pipeline, frame, constants.data(),
						0, dynamicOffsets.size(), dynamicOffsets.data(),
						externalDescriptorsSets.size(), externalDescriptorsSets.data(),
						*vertexes, *indexes, part.firstIndex, part.indexCount, part.firstVertex, part.instanceCount, part.firstInstance, part.vbOffset, part.ibOffset, indexType
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