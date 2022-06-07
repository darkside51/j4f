#pragma once

#include "../../Core/EngineModule.h"
#include <cstdint>

struct GLFWwindow;

namespace engine {
	class IRenderSurfaceInitialiser;

	class GLFWDevice : public IEngineModule {
	public:
		GLFWDevice();
		~GLFWDevice();

		const IRenderSurfaceInitialiser* getSurfaceInitialiser() const;
		void start();
		void stop();
		void leaveMainLoop();

		void setSize(const uint16_t w, const uint16_t h);
		GLFWwindow* getWindow() { return _window; }

		void swicthFullscreen(const bool fullscreen);

	private:
		GLFWwindow* _window;
		IRenderSurfaceInitialiser* _surfaceInitialiser;

		uint16_t _width;
		uint16_t _height;
	};
}