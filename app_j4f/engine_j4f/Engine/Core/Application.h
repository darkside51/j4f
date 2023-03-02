#pragma once

#include "Version.h"
#include <cstdint>

namespace engine {
	class ApplicationCustomData;
	class Application {
	public:
		Application() noexcept;
		~Application() { freeCustomData(); }
        void requestFeatures();
        void onEngineInitComplete();
		void update(const float delta);
		void nextFrame(const float delta);
		void resize(const uint16_t w, const uint16_t h);
		void deviceDestroyed() { freeCustomData(); }
        [[nodiscard]] Version version() const noexcept;
	private:
        void freeCustomData();
		ApplicationCustomData* _customData = nullptr;
	};

#ifdef J4F_NO_APPLICATION
	Application::Application() = default;
	void Application::update(const float delta) {}
	void Application::nextFrame(const float delta) {}
	void Application::resize(const uint16_t w, const uint16_t h) {}
	void Application::deviceDestroyed() {}
	void Application::freeCustomData() {}
#endif
}