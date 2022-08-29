#pragma once

#include "../../Core/Math/math.h"
#include "../Render/RenderedEntity.h"
#include "../Render/CommonVertexTypes.h"

namespace engine {
	
	class Plain : public RenderedEntity {
	public:
		Plain(const glm::vec2& sz, const vulkan::RenderDataGpuParamsType params = nullptr);
		void updateRenderData(const glm::mat4& worldMatrix, const bool worldMatrixChanged);

	private:
		glm::vec2 _size;
		TexturedVertex _vtx[4];
		uint32_t _idxs[6];
	};

}