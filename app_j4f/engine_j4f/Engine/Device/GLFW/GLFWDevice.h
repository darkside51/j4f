#pragma once

#include "../../Core/EngineModule.h"
#include <cstdint>

struct GLFWwindow;

namespace engine {
	class IRenderSurfaceInitializer;

	class GLFWDevice final : public IEngineModule {
	public:
		GLFWDevice();
		~GLFWDevice() override;

		[[nodiscard]] const IRenderSurfaceInitializer* getSurfaceInitializer() const;
		void start();
		void stop();
		void leaveMainLoop();

		void setSize(const uint16_t w, const uint16_t h);
		GLFWwindow* getWindow() { return _window; }

		void setFullscreen(const bool fullscreen);
        [[nodiscard]] bool isFullscreen() const;

        void setTittle(const char* value);
	private:
		GLFWwindow* _window = nullptr;
		IRenderSurfaceInitializer* _surfaceInitializer = nullptr;

		uint16_t _width = 0;
		uint16_t _height = 0;
		bool _fullscreen = false;
	};
}