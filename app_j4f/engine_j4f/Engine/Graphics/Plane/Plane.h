#pragma once

#include "../../Core/Math/mathematic.h"
#include "../Render/RenderedEntity.h"
#include "../Render/CommonVertexTypes.h"
#include "../Render/TextureFrame.h"

#include <memory>

namespace engine {
	
	class Plane : public RenderedEntity {
	public:
		Plane(const vec2f& sz, const vulkan::RenderDataGpuParamsType& params = nullptr);
		Plane(const std::shared_ptr<TextureFrame>& f, const vulkan::RenderDataGpuParamsType& params = nullptr);

		void updateRenderData(const mat4f& worldMatrix, const bool worldMatrixChanged);
		inline void updateModelMatrixChanged(const bool worldMatrixChanged) noexcept { _modelMatrixChanged |= worldMatrixChanged; }

		void setFrame(const std::shared_ptr<TextureFrame>& f);

	private:
		void createRenderData(const vulkan::RenderDataGpuParamsType& params);

		vec2f _aabb[2];
		std::vector<TexturedVertex> _vtx;
		std::vector<uint32_t> _idx;
		std::shared_ptr<TextureFrame> _frame = nullptr;
		bool _frameChanged = false;
		bool _modelMatrixChanged = true;
	};

}