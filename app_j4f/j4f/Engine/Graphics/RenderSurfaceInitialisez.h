#pragma once

#include <cstdint>

namespace engine {
	class IRenderSurfaceInitialiser {
	public:
		virtual ~IRenderSurfaceInitialiser() = default;
		virtual bool initRenderSurface(void* renderInstane, void* renderSurace) const = 0;
		virtual uint32_t getDesiredImageCount() const = 0;
	};
}