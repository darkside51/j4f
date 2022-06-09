#include "RenderDescriptor.h"
#include "RenderHelper.h"
#include "AutoBatchRender.h"
#include "../../Core/Engine.h"

namespace engine {

	void RenderDescriptor::render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const glm::mat4* cameraMatrix) {

		auto&& renderHelper = Engine::getInstance().getModule<Graphics>()->getRenderHelper();
		auto&& autoBatcher = renderHelper->getAutoBatchRenderer();

		switch (mode) {
			case engine::RenderDescritorMode::RDM_SINGLE_DRAW:
			{
				autoBatcher->draw(commandBuffer, currentFrame);

				for (uint32_t i = 0; i < renderDataCount; ++i) {
					vulkan::RenderData* r_data = renderData[i];
					if (r_data == nullptr || r_data->pipeline == nullptr || r_data->visible == false) continue;

					if (cameraMatrix && camera_matrix) {
						r_data->setRawDataForLayout(camera_matrix, const_cast<glm::mat4*>(cameraMatrix), false, sizeof(glm::mat4));
					}

					r_data->prepareRender(commandBuffer);
					r_data->render(commandBuffer, currentFrame, adittionalSetsCount, adittionalSets);
				}
			}
				break;
			case engine::RenderDescritorMode::RDM_AUTOBATCHING:
			{
				for (uint32_t i = 0; i < renderDataCount; ++i) {
					vulkan::RenderData* r_data = renderData[i];
					if (r_data == nullptr || r_data->pipeline == nullptr || r_data->visible == false) continue;

					if (cameraMatrix && camera_matrix) {
						r_data->setRawDataForLayout(camera_matrix, const_cast<glm::mat4*>(cameraMatrix), false, sizeof(glm::mat4));
					}

					autoBatcher->addToDraw(
						r_data->pipeline,
						batchingParams->vertexSize,
						batchingParams->vtxData,
						batchingParams->vtxDataSize,
						batchingParams->idxData,
						batchingParams->idxDataSize,
						r_data->params,
						commandBuffer,
						currentFrame
					);
				}
			}
				break;
			default:
				break;
		}
	}
}