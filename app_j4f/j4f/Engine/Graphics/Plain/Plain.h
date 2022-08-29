#pragma once

#include "../Render/RenderedEntity.h"

namespace engine {
	
	class Plain : public RenderedEntity {
	public:

		void updateRenderData(const glm::mat4& worldMatrix);
	private:
	};

}