#pragma once

#include "../Vulkan/vkRenderData.h"
#include "../../Core/Math/mathematic.h"
#include <cstdint>

namespace engine {

	enum class RenderDescritorMode : uint8_t {
		SINGLE_DRAW = 0,
		AUTOBATCHING = 1,
        CUSTOM_DRAW = 2
	};

    class IRenderDescriptorCustomRenderer {
    public:
        virtual ~IRenderDescriptorCustomRenderer() = default;
        virtual void render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const glm::mat4* cameraMatrix) = 0;
    };

	struct RenderDescriptor {
		uint32_t renderDataCount = 0;
		vulkan::RenderData** renderData = nullptr;
		RenderDescritorMode mode = RenderDescritorMode::SINGLE_DRAW;
		int16_t order = 0;
		bool visible = true;
        IRenderDescriptorCustomRenderer* customRenderer = nullptr;

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
		}

		inline void setRawDataForLayout(const vulkan::GPUParamLayoutInfo* info, void* value, const bool copyData, const size_t valueSize) const {
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