#include "Plain.h"
#include "../Render/RenderHelper.h"
#include "../Render/AutoBatchRender.h"

namespace engine {

	Plain::Plain(const glm::vec2& sz, const vulkan::RenderDataGpuParamsType params) : _size(sz) {
		auto&& renderHelper = Engine::getInstance().getModule<Graphics>()->getRenderHelper();
		auto&& pipeline = renderHelper->getPipeline(CommonPipelines::COMMON_PIPELINE_TEXTURED);

		const vulkan::RenderDataGpuParamsType gpu_params = params ? params : std::make_shared<engine::GpuProgramParams>();

		_renderDescriptor.mode = RenderDescritorMode::RDM_AUTOBATCHING;

		_renderDescriptor.renderData = new vulkan::RenderData*[1];
		_renderDescriptor.renderData[0] = new vulkan::RenderData(gpu_params);

		_renderDescriptor.renderDataCount = 1;

		_renderState.vertexDescription.bindings_strides.push_back(std::make_pair(0, sizeof(TexturedVertex)));
		_renderState.topology = { vulkan::TRIANGLE_LIST, false };
		_renderState.rasterisationState = vulkan::VulkanRasterizationState(vulkan::CULL_MODE_NONE, vulkan::POLYGON_MODE_FILL);
		_renderState.blendMode = vulkan::CommonBlendModes::blend_none;
		_renderState.depthState = vulkan::VulkanDepthState(true, true, VK_COMPARE_OP_LESS);
		_renderState.stencilState = vulkan::VulkanStencilState(false);

		_vertexInputAttributes = TexturedVertex::getVertexAttributesDescription();
		_renderState.vertexDescription.attributesCount = static_cast<uint32_t>(_vertexInputAttributes.size());
		_renderState.vertexDescription.attributes = _vertexInputAttributes.data();

		_fixedGpuLayouts.resize(1);
		_fixedGpuLayouts[0].second = "mvp";

		//
		_vtx[0] = { {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f} };
		_vtx[1] = { {_size.x, 0.0f, 0.0f}, {1.0f, 1.0f} };
		_vtx[2] = { {0.0f, _size.y, 0.0f}, {0.0f, 0.0f} };
		_vtx[3] = { {_size.x, _size.y, 0.0f}, {1.0f, 0.0f} };

		_idxs[0] = 0; _idxs[1] = 1; _idxs[2] = 2;
		_idxs[3] = 2; _idxs[4] = 1; _idxs[5] = 3;

		_renderDescriptor.batchingParams = new BatchingParams();
		_renderDescriptor.batchingParams->vertexSize = sizeof(TexturedVertex);
		_renderDescriptor.batchingParams->vtxDataSize = 4 * sizeof(TexturedVertex);
		_renderDescriptor.batchingParams->idxDataSize = 6 * sizeof(uint32_t);
		_renderDescriptor.batchingParams->vtxData = &_vtx[0];
		_renderDescriptor.batchingParams->idxData = &_idxs[0];

		setPipeline(const_cast<vulkan::VulkanPipeline*>(pipeline));
	}

	void Plain::updateRenderData(const glm::mat4& worldMatrix, const bool worldMatrixChanged) {
		if (worldMatrixChanged) {
			const glm::vec4 p0 = worldMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			const glm::vec4 p1 = worldMatrix * glm::vec4(_size.x, 0.0f, 0.0f, 1.0f);
			const glm::vec4 p2 = worldMatrix * glm::vec4(0.0f, _size.y, 0.0f, 1.0f);
			const glm::vec4 p3 = worldMatrix * glm::vec4(_size.x, _size.y, 0.0f, 1.0f);

			_vtx[0].position[0] = p0.x; _vtx[0].position[1] = p0.y; _vtx[0].position[2] = p0.z;
			_vtx[1].position[0] = p1.x; _vtx[1].position[1] = p1.y; _vtx[1].position[2] = p1.z;
			_vtx[2].position[0] = p2.x; _vtx[2].position[1] = p2.y; _vtx[2].position[2] = p2.z;
			_vtx[3].position[0] = p3.x; _vtx[3].position[1] = p3.y; _vtx[3].position[2] = p3.z;
		}
	}
}