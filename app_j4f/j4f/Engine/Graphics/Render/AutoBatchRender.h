#pragma once

#include "../Vulkan/vkRenderData.h"
#include "../../Core/Handler.h"

#include <vector>
#include <unordered_map>
#include <cstdint>

namespace engine {

	using GpuParamsType = std::shared_ptr<GpuProgramParams>;
	//using GpuParamsType = handler_ptr<GpuProgramParams>;

	class AutoBatchRenderer {
	public:

		~AutoBatchRenderer();

		void addToDraw(
			vulkan::VulkanPipeline* pipeline, 
			const size_t vertexSize,
			const void* vtxData, const uint32_t vtxDataSize, 
			const uint32_t* idxData, const uint32_t idxDataSize,
			const GpuParamsType& params,
			vulkan::VulkanCommandBuffer& commandBuffer,
			const uint32_t frame
		);

		inline void addToDraw(
			vulkan::RenderData* rdata,
			const size_t vertexSize,
			const void* vtxData,
			const uint32_t vtxDataSize, 
			const uint32_t* idxData,
			const uint32_t idxDataSize,
			vulkan::VulkanCommandBuffer& commandBuffer, 
			const uint32_t frame
		) {
			addToDraw(rdata->pipeline, vertexSize, vtxData, vtxDataSize, idxData, idxDataSize, rdata->params, commandBuffer, frame);
		}

		void draw(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t frame);

	private:
		vulkan::VulkanPipeline* _pipeline = nullptr;
		GpuParamsType _params;

		std::unordered_map<vulkan::VulkanPipeline*, vulkan::RenderData*> _render_data_map;
		std::vector<float> _vtx;
		std::vector<uint32_t> _idx;
	};

}