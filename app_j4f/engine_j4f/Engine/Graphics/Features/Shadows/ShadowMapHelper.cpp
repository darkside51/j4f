#include "ShadowMapHelper.h"
#include "CascadeShadowMap.h"
#include "../../Render/RenderList.h"

namespace engine {

	void renderShadowMap(CascadeShadowMap* shadowMap, RenderList& list, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame) {

		shadowMap->prepareToRender(commandBuffer);
		switch (shadowMap->techique()) {
			case ShadowMapTechnique::SMT_GEOMETRY_SH:
			{
				shadowMap->beginRenderPass(commandBuffer, 0);
				list.render(commandBuffer, currentFrame, nullptr);
				shadowMap->endRenderPass(commandBuffer);
			}
				break;
			default:
			{
				for (uint32_t i = 0; i < shadowMap->getCascadesCount(); ++i) {
					shadowMap->beginRenderPass(commandBuffer, i);

					//todo: проверять в какой каскад меш попадает и только там его и рисовать
					//mesh->setCameraMatrix(cascadesViewProjMatrixes[i]);
					//mesh2->setCameraMatrix(cascadesViewProjMatrixes[i]);
					//mesh3->setCameraMatrix(cascadesViewProjMatrixes[i]);

					//mesh->render(commandBuffer, currentFrame, &shadowMap->getVPMatrix(i));
					//mesh2->render(commandBuffer, currentFrame, &shadowMap->getVPMatrix(i));
					//mesh3->render(commandBuffer, currentFrame, &shadowMap->getVPMatrix(i));

					list.render(commandBuffer, currentFrame, &shadowMap->getVPMatrix(i));

					shadowMap->endRenderPass(commandBuffer);
				}
			}
				break;
		}

	}

	void renderShadowMap(CascadeShadowMap* shadowMap, RenderList** list, const uint32_t count, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame) {
		shadowMap->prepareToRender(commandBuffer);
		switch (shadowMap->techique()) {
			case ShadowMapTechnique::SMT_GEOMETRY_SH:
			{
				shadowMap->beginRenderPass(commandBuffer, 0);
				for (size_t j = 0; j < count; ++j) {
					list[j]->render(commandBuffer, currentFrame, nullptr);
				}
				shadowMap->endRenderPass(commandBuffer);
			}
				break;
			default:
			{
				for (uint32_t i = 0; i < shadowMap->getCascadesCount(); ++i) {
					shadowMap->beginRenderPass(commandBuffer, i);
					//todo: проверять в какой каскад меш попадает и только там его и рисовать
					for (size_t j = 0; j < count; ++j) {
						list[j]->render(commandBuffer, currentFrame, &shadowMap->getVPMatrix(i));
					}
					shadowMap->endRenderPass(commandBuffer);
				}
			}
				break;
		}
	}
}