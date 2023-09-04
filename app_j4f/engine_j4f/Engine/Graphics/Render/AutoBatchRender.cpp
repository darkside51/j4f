#include "AutoBatchRender.h"
#include "RenderHelper.h"
#include "../../Engine/Core/Engine.h"

namespace engine {

	AutoBatchRenderer::~AutoBatchRenderer() {
		for (auto it = _render_data_map.begin(); it != _render_data_map.end(); ++it) {
			it->second->setRenderParts(nullptr, 0);
			delete it->second;
		}
	}

	void AutoBatchRenderer::addToDraw(
		vulkan::VulkanPipeline* pipeline, 
		const size_t vertexSize,
		const void* vtxData, 
		const uint32_t vtxDataSize,
		const void* idxData,
		const uint32_t idxDataSize,
		const GpuParamsType& params,
		vulkan::VulkanCommandBuffer& commandBuffer, 
		const uint32_t frame
		) {

		if (_pipeline) {
			if (_pipeline != pipeline || _params != params) {
				draw(commandBuffer, frame);
			}
		}

		_pipeline = pipeline;
		_params = params;

		// add data to vtx & idx vectors
		const size_t vtxSize = _vtx.size();
		if (vtxDataSize > 0u) {
			_vtx.resize(vtxSize + vtxDataSize / sizeof(float));
			memcpy(&_vtx[vtxSize], vtxData, vtxDataSize);
		}

        const size_t idxSize = _idx.size();
        _idx.resize(idxSize + idxDataSize / sizeof(uint32_t));

        if (idxSize == 0) {
            memcpy(&_idx[idxSize], idxData, idxDataSize);
        } else {
            const size_t floatsPerVertex = vertexSize / sizeof(float);
            for (size_t i = 0; i < idxDataSize / sizeof(uint32_t); ++i) {
                _idx[idxSize + i] = static_cast<const uint32_t*>(idxData)[i] + vtxSize / floatsPerVertex;
            }
        }
	}

	void AutoBatchRenderer::draw(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t frame) {
		if (_vtx.empty()) return;
		size_t vOffset;
		size_t iOffset;

		const size_t idx_count = _idx.size();

		auto&& renderHelper = Engine::getInstance().getModule<Graphics>()->getRenderHelper();

		auto&& vBuffer = renderHelper->addDynamicVerteces(&_vtx[0], _vtx.size() * sizeof(float), vOffset);
		_vtx.clear();
		auto&& iBuffer = renderHelper->addDynamicIndices(
                &_idx[0],
                idx_count * sizeof(uint32_t),
                iOffset);

            _idx.clear();

		vulkan::RenderData* renderData;
		auto it = _render_data_map.find(_pipeline);
		if (it == _render_data_map.end()) {
			renderData = new vulkan::RenderData(_pipeline, _params);
			_render_data_map[_pipeline] = renderData;
		} else {
			renderData = it->second;
		}

		renderData->vertexes = &vBuffer;
		renderData->indexes = &iBuffer;
        renderData->indexType =  VK_INDEX_TYPE_UINT32;

		vulkan::RenderData::RenderPart renderPart{
													static_cast<uint32_t>(iOffset / sizeof(uint32_t)),	// firstIndex
													static_cast<uint32_t>(idx_count),					// indexCount
													0,													// vertexCount (parameter no used with indexed render)
													0,													// firstVertex
													1,													// instanceCount (can change later)
													0,													// firstInstance (can change later)
													vOffset,											// vbOffset
													0													// ibOffset
		};

		renderData->replaceParams(_params);
		renderData->setRenderParts(&renderPart, 1);
		renderData->prepareRender(/*commandBuffer*/);
		renderData->render(commandBuffer, frame);
		//renderData->setRenderParts(nullptr, 0);
	}

}