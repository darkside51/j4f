#include "RenderDescriptor.h"
#include "RenderHelper.h"
#include "AutoBatchRender.h"
#include "../Vulkan/vkDebugMarker.h"

namespace engine {

	void RenderDescriptor::render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const glm::mat4* cameraMatrix) {
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

					if (cameraMatrix && camera_matrix) {
						r_data->setRawDataForLayout(camera_matrix, const_cast<glm::mat4*>(cameraMatrix), false, sizeof(glm::mat4));
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

					if (cameraMatrix && camera_matrix) {
						r_data->setRawDataForLayout(camera_matrix, const_cast<glm::mat4*>(cameraMatrix), false, sizeof(glm::mat4));
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
                    customRenderer->render(commandBuffer, currentFrame, cameraMatrix);
                }

            }
				break;
			default:
				break;
		}
	}
}