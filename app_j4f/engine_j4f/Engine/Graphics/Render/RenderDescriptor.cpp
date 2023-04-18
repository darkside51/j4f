#include "RenderDescriptor.h"
#include "RenderHelper.h"
#include "AutoBatchRender.h"
#include "../Vulkan/vkDebugMarker.h"

namespace engine {

	void RenderDescriptor::render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const ViewParams& viewParams) {
		GPU_DEBUG_MARKER_INSERT(commandBuffer.m_commandBuffer, " j4f renderDescriptor render", 0.5f, 0.5f, 0.5f, 1.0f);

		auto&& renderHelper = Engine::getInstance().getModule<Graphics>()->getRenderHelper();
		auto&& autoBatcher = renderHelper->getAutoBatchRenderer();

		switch (mode) {
		case RenderDescritorMode::SINGLE_DRAW:
		{
			autoBatcher->draw(commandBuffer, currentFrame);

			for (uint32_t i = 0; i < renderDataCount; ++i) {
				vulkan::RenderData* r_data = renderData[i];
				if (r_data == nullptr || r_data->pipeline == nullptr || !r_data->visible) continue;

				uint8_t p = 0;
				for (auto& param : viewParamsLayouts) {
					if (param) {
						r_data->setRawDataForLayout(param, const_cast<glm::mat4*>(viewParams[p]), false, sizeof(glm::mat4));
					}
					++p;
				}

				r_data->prepareRender(/*commandBuffer*/);
				r_data->render(commandBuffer, currentFrame);
			}
		}
		break;
		case RenderDescritorMode::AUTOBATCHING:
		{
			for (uint32_t i = 0; i < renderDataCount; ++i) {
				vulkan::RenderData* r_data = renderData[i];
				if (r_data == nullptr || r_data->pipeline == nullptr || !r_data->visible) continue;

				uint8_t p = 0;
				for (auto& param : viewParamsLayouts) {
					if (param) {
						r_data->setRawDataForLayout(param, const_cast<glm::mat4*>(viewParams[p]), false, sizeof(glm::mat4));
					}
					++p;
				}

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
				customRenderer->render(commandBuffer, currentFrame, viewParams);
			}
		}
		break;
		default:
			break;
		}
	}
}