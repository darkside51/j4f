// ♣♠♦♥
#include "ShadowMapHelper.h"
#include "CascadeShadowMap.h"
#include "../../Render/RenderList.h"
#include "../../Render/RenderDescriptor.h"

namespace engine {

	void renderShadowMap(CascadeShadowMap* shadowMap, RenderList& list, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame) {

		shadowMap->prepareToRender(commandBuffer);
		switch (shadowMap->techique()) {
			case ShadowMapTechnique::SMT_GEOMETRY_SH:
			{
				shadowMap->beginRenderPass(commandBuffer, 0u);
				list.render(commandBuffer, currentFrame, { nullptr, nullptr, nullptr });
				shadowMap->endRenderPass(commandBuffer);
			}
				break;
            case ShadowMapTechnique::SMT_INSTANCE_DRAW:
            {
                shadowMap->beginRenderPass(commandBuffer, 0u);
                list.render(commandBuffer, currentFrame,
                            { nullptr, nullptr, nullptr },
                            shadowMap->getCascadesCount());
                shadowMap->endRenderPass(commandBuffer);
            }
                break;
			default:
			{
                // todo: check if object completed render in shadow layer - no draw it to next layers
				for (uint32_t i = 0; i < shadowMap->getCascadesCount(); ++i) {
					shadowMap->beginRenderPass(commandBuffer, i);

					//todo: ��������� � ����� ������ ��� �������� � ������ ��� ��� � ��������
					//mesh->setCameraMatrix(cascadesViewProjMatrixes[i]);
					//mesh2->setCameraMatrix(cascadesViewProjMatrixes[i]);
					//mesh3->setCameraMatrix(cascadesViewProjMatrixes[i]);

					//mesh->render(commandBuffer, currentFrame, &shadowMap->getVPMatrix(i));
					//mesh2->render(commandBuffer, currentFrame, &shadowMap->getVPMatrix(i));
					//mesh3->render(commandBuffer, currentFrame, &shadowMap->getVPMatrix(i));

					list.render(commandBuffer, currentFrame, { &shadowMap->getVPMatrix(i), nullptr, nullptr });

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
				shadowMap->beginRenderPass(commandBuffer, 0u);
				for (size_t j = 0; j < count; ++j) {
					list[j]->render(commandBuffer, currentFrame, { nullptr, nullptr, nullptr });
				}
				shadowMap->endRenderPass(commandBuffer);
			}
				break;
            case ShadowMapTechnique::SMT_INSTANCE_DRAW:
            {
                shadowMap->beginRenderPass(commandBuffer, 0u);
                for (size_t j = 0; j < count; ++j) {
                    list[j]->render(commandBuffer, currentFrame,
                                    { nullptr, nullptr, nullptr },
                                    shadowMap->getCascadesCount());
                }
                shadowMap->endRenderPass(commandBuffer);
            }
                break;
			default:
			{
				for (uint32_t i = 0; i < shadowMap->getCascadesCount(); ++i) {
					shadowMap->beginRenderPass(commandBuffer, i);
                    // todo: check if object completed render in shadow layer - no draw it to next layers
					//todo: ��������� � ����� ������ ��� �������� � ������ ��� ��� � ��������
					for (size_t j = 0; j < count; ++j) {
						list[j]->render(commandBuffer, currentFrame, { &shadowMap->getVPMatrix(i), nullptr, nullptr });
					}
					shadowMap->endRenderPass(commandBuffer);
				}
			}
				break;
		}
	}
}