#pragma once

#include "../../Core/Math/mathematic.h"
#include "../Render/RenderedEntity.h"
#include "../Render/CommonVertexTypes.h"
#include "../Render/TextureFrame.h"

#include <memory>

namespace engine {
	
	class Plane : public RenderedEntity {
	public:
		Plane(const glm::vec2& sz, const vulkan::RenderDataGpuParamsType& params = nullptr);
		Plane(const std::shared_ptr<TextureFrame>& f, const vulkan::RenderDataGpuParamsType& params = nullptr);

		void updateRenderData(const glm::mat4& worldMatrix, const bool worldMatrixChanged);
		inline void updateModelMatrixChanged(const bool worldMatrixChanged) noexcept { _modelMatrixChanged |= worldMatrixChanged; }

		void setFrame(const std::shared_ptr<TextureFrame>& f);

	private:
		void createRenderData(const vulkan::RenderDataGpuParamsType& params);

		glm::vec2 _aabb[2];
		std::vector<TexturedVertex> _vtx;
		std::vector<uint32_t> _idx;
		std::shared_ptr<TextureFrame> _frame = nullptr;
		bool _frameChanged = false;
		bool _modelMatrixChanged = true;
	};

}