#include "RenderDescriptor.h"
#include "RenderHelper.h"
#include "AutoBatchRender.h"
#include "../Vulkan/vkDebugMarker.h"

namespace engine {

	void RenderDescriptor::render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const ViewParams& viewParams, const uint16_t drawCount) {
		GPU_DEBUG_MARKER_INSERT(commandBuffer.m_commandBuffer, " j4f renderDescriptor render", 0.5f, 0.5f, 0.5f, 1.0f);

		auto&& renderHelper = Engine::getInstance().getModule<Graphics>().getRenderHelper();
		auto&& autoBatcher = renderHelper->getAutoBatchRenderer();

		switch (mode) {
		case RenderDescritorMode::SINGLE_DRAW:
		{
			autoBatcher->draw(commandBuffer, currentFrame);

			for (uint32_t i = 0; i < renderDataCount; ++i) {
				vulkan::RenderData* r_data = renderData[i];
				if (r_data == nullptr || r_data->pipeline == nullptr || !r_data->visible) continue;

				const auto* p = viewParams.data();
				for (auto& param : viewParamsLayouts) {
					if (param) {
						r_data->setRawDataForLayout(param, const_cast<mat4f*>(p), false, sizeof(mat4f));
					}
					++p;
				}

                const auto instanceCount = r_data->instanceCount;
                if (drawCount != 1u) {
                    r_data->instanceCount *= drawCount;
                }

				r_data->prepareRender(/*commandBuffer*/);
				r_data->render(commandBuffer, currentFrame);

                if (drawCount != 1u) {
                    r_data->instanceCount = instanceCount;
                }
			}
		}
		break;
		case RenderDescritorMode::AUTO_BATCHING:
		{
			for (uint32_t i = 0; i < renderDataCount; ++i) {
				vulkan::RenderData* r_data = renderData[i];
				if (r_data == nullptr || r_data->pipeline == nullptr || !r_data->visible) continue;

				const auto* p = viewParams.data();
				for (auto& param : viewParamsLayouts) {
					if (param) {
						r_data->setRawDataForLayout(param, const_cast<mat4f*>(p), false, sizeof(mat4f));
					}
					++p;
				}

                // no support drawCount in autoBatcher
				autoBatcher->addToDraw(
					r_data->pipeline,
					r_data->vertexSize,
					r_data->batchingParams->rawVertexes,
					r_data->batchingParams->vtxDataSize,
					r_data->batchingParams->rawIndexes,
					r_data->batchingParams->idxDataSize,
					r_data->params,
					commandBuffer,
					currentFrame
				);
			}
		}
		break;
		case RenderDescritorMode::CUSTOM_DRAW:
		{
			if (customRenderer) {
				autoBatcher->draw(commandBuffer, currentFrame);
				customRenderer->render(commandBuffer, currentFrame, viewParams, drawCount);
			}
		}
		break;
		default:
			break;
		}
	}
}