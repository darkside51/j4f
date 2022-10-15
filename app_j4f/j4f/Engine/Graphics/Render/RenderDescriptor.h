#pragma once

#include "../Vulkan/vkRenderData.h"
#include "../../Core/Math/math.h"
#include <cstdint>

namespace engine {

	enum class RenderDescritorMode : uint8_t {
		RDM_SINGLE_DRAW = 0,
		RDM_AUTOBATCHING = 1
	};

	struct BatchingParams {
		size_t vertexSize;
		uint32_t vtxDataSize;
		uint32_t idxDataSize;
		void* vtxData;
		uint32_t* idxData;
	};

	struct RenderDescriptor {
		uint32_t renderDataCount = 0;
		vulkan::RenderData** renderData = nullptr;
		BatchingParams* batchingParams = nullptr;
		RenderDescritorMode mode = RenderDescritorMode::RDM_SINGLE_DRAW;
		int16_t order = 0;
		bool visible = true;

		const vulkan::GPUParamLayoutInfo* camera_matrix = nullptr;

		~RenderDescriptor() { destroy(); }

		inline void setCameraMatrix(const vulkan::GPUParamLayoutInfo* layout) { camera_matrix = layout; }

		void destroy() {
			if (renderDataCount) {
				for (uint32_t i = 0; i < renderDataCount; ++i) {
					delete renderData[i];
				}

				delete[] renderData;
				renderDataCount = 0;
			}

			if (batchingParams) {
				delete batchingParams;
				batchingParams = nullptr;
			}
		}

		inline void setRawDataForLayout(const vulkan::GPUParamLayoutInfo* info, void* value, const bool copyData, const size_t valueSize) {
			for (uint32_t i = 0; i < renderDataCount; ++i) {
				vulkan::RenderData* r_data = renderData[i];
				if (r_data == nullptr || r_data->pipeline == nullptr) continue;

				r_data->setRawDataForLayout(info, value, copyData, valueSize);
			}
		}

		template <typename T>
		inline void setParamForLayout(const vulkan::GPUParamLayoutInfo* info, T* value, const bool copyData, const uint32_t count = 1) {
			for (uint32_t i = 0; i < renderDataCount; ++i) {
				vulkan::RenderData* r_data = renderData[i];
				if (r_data == nullptr || r_data->pipeline == nullptr) continue;

				r_data->setParamForLayout(info, value, copyData, count);
			}
		}

		template <typename T>
		inline void setParamByName(const std::string& name, T* value, bool copyData, const uint32_t count = 1) {
			for (uint32_t i = 0; i < renderDataCount; ++i) {
				vulkan::RenderData* r_data = renderData[i];
				if (r_data == nullptr || r_data->pipeline == nullptr) continue;

				r_data->setParamByName(name, value, copyData, count);
			}
		}

		void render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const glm::mat4* cameraMatrix);
	};
}