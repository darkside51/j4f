#include "Plain.h"
#include "../Render/RenderHelper.h"
#include "../Render/AutoBatchRender.h"

namespace engine {

	Plain::Plain(const glm::vec2& sz, const vulkan::RenderDataGpuParamsType& params) {
        createRenderData(params);
		_aabb[0] = glm::vec2(0.0f);
		_aabb[1] = glm::vec2(sz);
		
		_vtx = {
			{ {_aabb[0].x, _aabb[0].y, 0.0f}, {0.0f, 1.0f} },
			{ {_aabb[1].x, _aabb[0].y, 0.0f}, {1.0f, 1.0f} },
			{ {_aabb[0].x, _aabb[1].y, 0.0f}, {0.0f, 0.0f} },
			{ {_aabb[1].x, _aabb[1].y, 0.0f}, {1.0f, 0.0f} }
		};

		_idx = { 0, 1, 2, 2, 1, 3 };

        _renderDescriptor.renderData[0]->batchingParams = new vulkan::BatchingParams();
        _renderDescriptor.renderData[0]->vertexSize = sizeof(TexturedVertex);
        _renderDescriptor.renderData[0]->batchingParams->vtxDataSize = _vtx.size() * sizeof(TexturedVertex);
        _renderDescriptor.renderData[0]->batchingParams->idxDataSize = _idx.size() * sizeof(uint32_t);
        _renderDescriptor.renderData[0]->batchingParams->rawVertexes = _vtx.data();
        _renderDescriptor.renderData[0]->batchingParams->rawIndexes = _idx.data();
	}

	Plain::Plain(const std::shared_ptr<TextureFrame>& f, const vulkan::RenderDataGpuParamsType& params) : _frame(f) {
        createRenderData(params);

		_vtx.resize(_frame->_vtx.size() / 2);
		_idx = _frame->_idx;

		size_t j = 0;
		for (size_t i = 0, sz = _frame->_vtx.size(); i < sz; i += 2) {
			_vtx[j].uv[0] = _frame->_uv[i]; 
			_vtx[j].uv[1] = _frame->_uv[i + 1];
			++j;
		}
		_frameChanged = true;

        _renderDescriptor.renderData[0]->batchingParams = new vulkan::BatchingParams();
        _renderDescriptor.renderData[0]->vertexSize = sizeof(TexturedVertex);
        _renderDescriptor.renderData[0]->batchingParams->vtxDataSize = _vtx.size() * sizeof(TexturedVertex);
        _renderDescriptor.renderData[0]->batchingParams->idxDataSize = _idx.size() * sizeof(uint32_t);
        _renderDescriptor.renderData[0]->batchingParams->rawVertexes = _vtx.data();
        _renderDescriptor.renderData[0]->batchingParams->rawIndexes = _idx.data();
	}

	void Plain::setFrame(const std::shared_ptr<TextureFrame>& f) {
		if (_frame == f) { return; }

		_frame = f;
		_vtx.resize(_frame->_vtx.size() / 2);
		_idx = _frame->_idx;

		size_t j = 0;
		for (size_t i = 0, sz = _frame->_vtx.size(); i < sz; i += 2) {
			_vtx[j].uv[0] = _frame->_uv[i];
			_vtx[j].uv[1] = _frame->_uv[i + 1];
			++j;
		}

		_frameChanged = true;

        _renderDescriptor.renderData[0]->batchingParams->vtxDataSize = _vtx.size() * sizeof(TexturedVertex);
        _renderDescriptor.renderData[0]->batchingParams->idxDataSize = _idx.size() * sizeof(uint32_t);
        _renderDescriptor.renderData[0]->batchingParams->rawVertexes = _vtx.data();
        _renderDescriptor.renderData[0]->batchingParams->rawIndexes = _idx.data();

	}

	void Plain::createRenderData(const vulkan::RenderDataGpuParamsType& params) {
		auto&& renderHelper = Engine::getInstance().getModule<Graphics>()->getRenderHelper();
		auto&& pipeline = renderHelper->getPipeline(CommonPipelines::COMMON_PIPELINE_TEXTURED);

		const vulkan::RenderDataGpuParamsType gpu_params = params ? params : std::make_shared<engine::GpuProgramParams>();

		_renderDescriptor.mode = RenderDescritorMode::AUTOBATCHING;

        _renderDescriptor.renderDataCount = 1;
		_renderDescriptor.renderData = new vulkan::RenderData *[1];
		_renderDescriptor.renderData[0] = new vulkan::RenderData(gpu_params);

		_renderState.vertexDescription.bindings_strides.push_back(std::make_pair(0, sizeof(TexturedVertex)));
		_renderState.topology = { vulkan::PrimitiveTopology::TRIANGLE_LIST, false };
		_renderState.rasterizationState = vulkan::VulkanRasterizationState(vulkan::CullMode::CULL_MODE_NONE, vulkan::PoligonMode::POLYGON_MODE_FILL);
		_renderState.blendMode = vulkan::CommonBlendModes::blend_none;
		_renderState.depthState = vulkan::VulkanDepthState(true, true, VK_COMPARE_OP_LESS);
		_renderState.stencilState = vulkan::VulkanStencilState(false);

		_vertexInputAttributes = TexturedVertex::getVertexAttributesDescription();
		_renderState.vertexDescription.attributesCount = static_cast<uint32_t>(_vertexInputAttributes.size());
		_renderState.vertexDescription.attributes = _vertexInputAttributes.data();

		_fixedGpuLayouts.resize(1);
		_fixedGpuLayouts[0].second = { "mvp", ViewParams::Ids::CAMERA_TRANSFORM };

		setPipeline(Engine::getInstance().getModule<Graphics>()->getRenderer()->getGraphicsPipeline(_renderState, pipeline->program));
	}

	void Plain::updateRenderData(const glm::mat4& worldMatrix, const bool worldMatrixChanged) {
		_modelMatrixChanged |= worldMatrixChanged;

		if (_modelMatrixChanged || _frameChanged) {
			_frameChanged = false;
			if (_frame) {
				size_t j = 0;
				for (size_t i = 0, sz = _frame->_vtx.size(); i < sz; i += 2) {
					const glm::vec4 p = worldMatrix * glm::vec4(_frame->_vtx[i], _frame->_vtx[i + 1], 0.0f, 1.0f);
					_vtx[j].position[0] = p.x; _vtx[j].position[1] = p.y; _vtx[j].position[2] = p.z;
					++j;
				}
			} else {
				const glm::vec4 p0 = worldMatrix * glm::vec4(_aabb[0].x, _aabb[0].y, 0.0f, 1.0f);
				const glm::vec4 p1 = worldMatrix * glm::vec4(_aabb[1].x, _aabb[0].y, 0.0f, 1.0f);
				const glm::vec4 p2 = worldMatrix * glm::vec4(_aabb[0].x, _aabb[1].y, 0.0f, 1.0f);
				const glm::vec4 p3 = worldMatrix * glm::vec4(_aabb[1].x, _aabb[1].y, 0.0f, 1.0f);

				_vtx[0].position[0] = p0.x; _vtx[0].position[1] = p0.y; _vtx[0].position[2] = p0.z;
				_vtx[1].position[0] = p1.x; _vtx[1].position[1] = p1.y; _vtx[1].position[2] = p1.z;
				_vtx[2].position[0] = p2.x; _vtx[2].position[1] = p2.y; _vtx[2].position[2] = p2.z;
				_vtx[3].position[0] = p3.x; _vtx[3].position[1] = p3.y; _vtx[3].position[2] = p3.z;
			}

			_modelMatrixChanged = false;
		}
	}
}