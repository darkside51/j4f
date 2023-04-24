#pragma once

#include "../Vulkan/vkRenderData.h"
#include "../../Core/Math/mathematic.h"

#include <array>
#include <cstdint>

namespace engine {

	enum class RenderDescritorMode : uint8_t {
		SINGLE_DRAW = 0,
		AUTOBATCHING = 1,
        CUSTOM_DRAW = 2
	};

	struct ViewParams {
		enum class Ids : uint8_t {
			CAMERA_TRANSFORM = 0,
			VIEW_TRANSFORM = 1,
			PROJECTION_TRANSFROM = 2,
			UNKNOWN = 0xff
		};

		struct Params {
			const glm::mat4* cameraTransfrom = nullptr;
			const glm::mat4* cameraViewTransfrom = nullptr;
			const glm::mat4* cameraProjectionTransfrom = nullptr;
		};

		union {
			Params params;
			std::array<const glm::mat4*, 3> values;
		} v;

		const glm::mat4* operator[](const uint8_t i) const {
			return v.values[i];
		}

		const glm::mat4* data() const {
			return *v.values.data();
		}
	};

    class IRenderDescriptorCustomRenderer {
    public:
        virtual ~IRenderDescriptorCustomRenderer() = default;
		virtual void render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const ViewParams& viewParams) = 0;
    };

	struct RenderDescriptor {
		uint32_t renderDataCount = 0;
		vulkan::RenderData** renderData = nullptr;
		RenderDescritorMode mode = RenderDescritorMode::SINGLE_DRAW;
		int16_t order = 0;
		bool visible = true;
        IRenderDescriptorCustomRenderer* customRenderer = nullptr;
		std::array<const vulkan::GPUParamLayoutInfo*, 3> viewParamsLayouts = {nullptr, nullptr, nullptr};

		~RenderDescriptor() { destroy(); }

		inline void setViewParamLayout(const vulkan::GPUParamLayoutInfo* layout, const ViewParams::Ids idx) { viewParamsLayouts[static_cast<uint8_t>(idx)] = layout; }

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

		void render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const ViewParams& viewParams);
	};
}