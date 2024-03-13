#include "Plane.h"
#include "../Render/RenderHelper.h"
#include "../Render/AutoBatchRender.h"

namespace engine {

	Plane::Plane(const vec2f& sz, const engine::GpuParamsType& params) {
        createRenderData(params);
		_aabb[0] = vec2f(0.0f);
		_aabb[1] = vec2f(sz);
		
		_vtx = {
			{ {_aabb[0].x, _aabb[0].y, 0.0f}, {0.0f, 1.0f} },
			{ {_aabb[1].x, _aabb[0].y, 0.0f}, {1.0f, 1.0f} },
			{ {_aabb[0].x, _aabb[1].y, 0.0f}, {0.0f, 0.0f} },
			{ {_aabb[1].x, _aabb[1].y, 0.0f}, {1.0f, 0.0f} }
		};

		_idx = { 0u, 1u, 2u, 2u, 1u, 3u };

        _renderDescriptor.renderData[0]->batchingParams = new vulkan::BatchingParams();
        _renderDescriptor.renderData[0]->vertexSize = sizeof(TexturedVertex);
        _renderDescriptor.renderData[0]->batchingParams->vtxDataSize = _vtx.size() * sizeof(TexturedVertex);
        _renderDescriptor.renderData[0]->batchingParams->idxDataSize = _idx.size() * sizeof(uint32_t);
        _renderDescriptor.renderData[0]->batchingParams->rawVertexes = _vtx.data();
        _renderDescriptor.renderData[0]->batchingParams->rawIndexes = _idx.data();
	}

	Plane::Plane(const std::shared_ptr<TextureFrame>& f, const engine::GpuParamsType& params) : _frame(f) {
        createRenderData(params);

		_vtx.resize(_frame->_vtx.size() / 2);
		_idx = _frame->_idx;

		size_t j = 0u;
		for (size_t i = 0u, sz = _frame->_vtx.size(); i < sz; i += 2u) {
			_vtx[j].uv[0u] = _frame->_uv[i];
			_vtx[j].uv[1u] = _frame->_uv[i + 1u];
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

	void Plane::setFrame(const std::shared_ptr<TextureFrame>& f) {
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

	void Plane::createRenderData(const engine::GpuParamsType& params) {
		auto&& renderHelper = Engine::getInstance().getModule<Graphics>().getRenderHelper();
		auto&& pipeline = renderHelper->getPipeline(CommonPipelines::COMMON_PIPELINE_TEXTURED);

		const engine::GpuParamsType gpu_params = params ? params : engine::make_linked<engine::GpuProgramParams>();

		_renderDescriptor.mode = RenderDescriptorMode::AUTO_BATCHING;

		_renderDescriptor.renderData.push_back(std::make_unique<vulkan::RenderData>(gpu_params));

		_renderState.vertexDescription.bindings_strides.push_back(std::make_pair(0, sizeof(TexturedVertex)));
		_renderState.topology = { vulkan::PrimitiveTopology::TRIANGLE_LIST, false };
		_renderState.rasterizationState = vulkan::VulkanRasterizationState(vulkan::CullMode::CULL_MODE_NONE, vulkan::PolygonMode::POLYGON_MODE_FILL);
		_renderState.blendMode = vulkan::CommonBlendModes::blend_none;
		_renderState.depthState = vulkan::VulkanDepthState(true, true, VK_COMPARE_OP_LESS);
		_renderState.stencilState = vulkan::VulkanStencilState(false);
		_renderState.vertexDescription.attributes = TexturedVertex::getVertexAttributesDescription();

		_fixedGpuLayouts.resize(1);
		_fixedGpuLayouts[0].second = { "mvp", ViewParams::Ids::CAMERA_TRANSFORM };

		setPipeline(Engine::getInstance().getModule<Graphics>().getRenderer()->getGraphicsPipeline(_renderState, pipeline->program));
	}

	void Plane::updateRenderData(RenderDescriptor & /*renderDescriptor*/, const mat4f& worldMatrix, const bool worldMatrixChanged) {
		_modelMatrixChanged |= worldMatrixChanged;

		if (_modelMatrixChanged || _frameChanged) {
			_frameChanged = false;
			if (_frame) {
				size_t j = 0;
				for (size_t i = 0, sz = _frame->_vtx.size(); i < sz; i += 2) {
					const vec4f p = worldMatrix * vec4f(_frame->_vtx[i], _frame->_vtx[i + 1], 0.0f, 1.0f);
					_vtx[j].position[0] = p.x; _vtx[j].position[1] = p.y; _vtx[j].position[2] = p.z;
					++j;
				}
			} else {
				const vec4f p0 = worldMatrix * vec4f(_aabb[0].x, _aabb[0].y, 0.0f, 1.0f);
				const vec4f p1 = worldMatrix * vec4f(_aabb[1].x, _aabb[0].y, 0.0f, 1.0f);
				const vec4f p2 = worldMatrix * vec4f(_aabb[0].x, _aabb[1].y, 0.0f, 1.0f);
				const vec4f p3 = worldMatrix * vec4f(_aabb[1].x, _aabb[1].y, 0.0f, 1.0f);

				_vtx[0].position[0] = p0.x; _vtx[0].position[1] = p0.y; _vtx[0].position[2] = p0.z;
				_vtx[1].position[0] = p1.x; _vtx[1].position[1] = p1.y; _vtx[1].position[2] = p1.z;
				_vtx[2].position[0] = p2.x; _vtx[2].position[1] = p2.y; _vtx[2].position[2] = p2.z;
				_vtx[3].position[0] = p3.x; _vtx[3].position[1] = p3.y; _vtx[3].position[2] = p3.z;
			}

			_modelMatrixChanged = false;
		}
	}
}