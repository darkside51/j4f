#pragma once

#include "../Vulkan/vkRenderData.h"
#include "../../Core/Math/mathematic.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace engine {

	enum class RenderDescritorMode : uint8_t {
		SINGLE_DRAW = 0u,
		AUTO_BATCHING = 1u,
        CUSTOM_DRAW = 2u
	};

	struct ViewParams {
		enum class Ids : uint8_t {
			CAMERA_TRANSFORM = 0u,
			VIEW_TRANSFORM = 1u,
			PROJECTION_TRANSFROM = 2u,
			UNKNOWN = 0xffu
		};

		struct Params {
			const mat4f* cameraTransfrom = nullptr;
			const mat4f* cameraViewTransfrom = nullptr;
			const mat4f* cameraProjectionTransfrom = nullptr;
		};

		union {
			Params params;
			std::array<const mat4f*, 3u> values;
		} v;

		const mat4f* operator[](const uint8_t i) const {
			return v.values[i];
		}

		const mat4f* data() const {
			return *v.values.data();
		}
	};

    class IRenderDescriptorCustomRenderer {
    public:
        virtual ~IRenderDescriptorCustomRenderer() = default;
		virtual void render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const ViewParams& viewParams, const uint16_t drawCount = 1u) = 0;
    };

	struct RenderDescriptor {
        std::vector<std::unique_ptr<vulkan::RenderData>> renderData;
		RenderDescritorMode mode = RenderDescritorMode::SINGLE_DRAW;
		int16_t order = 0;
		bool visible = true;
        IRenderDescriptorCustomRenderer* customRenderer = nullptr;
		std::array<const vulkan::GPUParamLayoutInfo*, 3> viewParamsLayouts = {nullptr, nullptr, nullptr};

		~RenderDescriptor() { destroy(); }

		inline void setViewParamLayout(const vulkan::GPUParamLayoutInfo* layout, const ViewParams::Ids idx) { viewParamsLayouts[static_cast<uint8_t>(idx)] = layout; }

		void destroy() {
			renderData.clear();
		}

		inline void setRawDataForLayout(const vulkan::GPUParamLayoutInfo* info, void* value, const bool copyData, const size_t valueSize) const {
			for (auto&& r_data : renderData) {
				if (r_data == nullptr || r_data->pipeline == nullptr) {
                    continue;
                }

				r_data->setRawDataForLayout(info, value, copyData, valueSize);
			}
		}

		template <typename T>
		inline void setParamForLayout(const vulkan::GPUParamLayoutInfo* info, T* value, const bool copyData, const uint32_t count = 1) {
            for (auto&& r_data : renderData) {
				if (r_data == nullptr || r_data->pipeline == nullptr) {
                    continue;
                }

				r_data->setParamForLayout(info, value, copyData, count);
			}
		}

		template <typename T>
		inline void setParamByName(const std::string& name, T* value, bool copyData, const uint32_t count = 1u) {
            for (auto&& r_data : renderData) {
				if (r_data == nullptr || r_data->pipeline == nullptr) {
                    continue;
                }

				r_data->setParamByName(name, value, copyData, count);
			}
		}

		void render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const ViewParams& viewParams, const uint16_t drawCount);
	};
}