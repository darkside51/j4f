#include "BoundingVolume.h"
#include "../Render/RenderDescriptor.h"
#include "../Render/RenderHelper.h"

namespace engine {

#ifdef ENABLE_DRAW_BOUNDING_VOLUMES
	void SphereVolume::createDebugRenderDescriptor() {
		_debugRenderDescriptor = new RenderDescriptor();
		_debugRenderDescriptor->mode = RenderDescritorMode::RDM_AUTOBATCHING;
		_debugRenderDescriptor->renderDataCount = 1;

		auto&& renderHelper = Engine::getInstance().getModule<Graphics>()->getRenderHelper();

		auto&& pipeline = renderHelper->getPipeline(CommonPipelines::COMMON_PIPELINE_LINES);
		const vulkan::GPUParamLayoutInfo* mvp_layout = pipeline->program->getGPUParamLayoutByName("mvp");

		_debugRenderDescriptor->renderData = new vulkan::RenderData*[1];
		_debugRenderDescriptor->renderData[0] = new vulkan::RenderData(const_cast<vulkan::VulkanPipeline*>(pipeline));
		_debugRenderDescriptor->setCameraMatrix(mvp_layout);

		/*
		constexpr uint8_t segments = 64;
		constexpr float a = math_constants::pi2 / segments;

		ColoredVertex vtx[3 * segments];
		uint32_t idxs[3 * segments * 2];

		const glm::vec4 center = worldMatrix * glm::vec4(c.x, c.y, c.z, 1.0f);
		const float radius = glm::length(worldMatrix[0]) * r;

		uint8_t vtxCount = 0;
		float angle = 0.0f;
		for (uint8_t i = 0; i < segments; ++i) {
			const float x = center.x + radius * cosf(angle);
			const float y = center.y + radius * sinf(angle);
			const float z = center.z;

			const uint8_t j = vtxCount + i;

			vtx[j] = { {x, y, z}, {0.0f, 0.0f, 1.0f} };

			idxs[2 * j] = j;
			idxs[2 * j + 1] = i == (segments - 1) ? vtxCount : j + 1;

			angle += a;
		}

		vtxCount += segments;
		angle = 0.0f;
		for (uint8_t i = 0; i < segments; ++i) {
			const float x = center.x + radius * cosf(angle);
			const float y = center.y;
			const float z = center.z + radius * sinf(angle);

			const uint8_t j = vtxCount + i;

			vtx[j] = { {x, y, z}, {0.0f, 1.0f, 0.0f} };

			idxs[2 * j] = j;
			idxs[2 * j + 1] = i == (segments - 1) ? vtxCount : j + 1;

			angle += a;
		}

		vtxCount += segments;
		angle = 0.0f;
		for (uint8_t i = 0; i < segments; ++i) {
			const float x = center.x;
			const float y = center.y + radius * sinf(angle);
			const float z = center.z + radius * cosf(angle);

			const uint8_t j = vtxCount + i;

			vtx[j] = { {x, y, z}, {1.0f, 0.0f, 0.0f} };

			idxs[2 * j] = j;
			idxs[2 * j + 1] = i == (segments - 1) ? vtxCount : j + 1;

			angle += a;
		}

		constexpr uint32_t vertexBufferSize = 3 * segments * sizeof(ColoredVertex);
		constexpr uint32_t indexBufferSize = 3 * 2 * segments * sizeof(uint32_t);
		*/
	}
#endif // ENABLE_DRAW_BOUNDING_VOLUMES

}