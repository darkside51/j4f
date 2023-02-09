#pragma once

#include <cstdint>

namespace engine {
	class IRenderSurfaceInitializer {
	public:
		virtual ~IRenderSurfaceInitializer() = default;
		virtual bool initRenderSurface(void* renderInstane, void* renderSurace) const = 0;
		[[nodiscard]] virtual uint32_t getDesiredImageCount() const = 0;
	};
}