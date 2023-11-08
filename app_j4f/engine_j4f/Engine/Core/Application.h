#pragma once

#include "Version.h"
#include <cstdint>
#include <memory>

namespace engine {
	class Game;
	class Application {
	public:
		Application() noexcept;
		~Application();
        void requestFeatures();
        void onEngineInitComplete();
		void update(const float delta);
		void render(const float delta);
		void resize(const uint16_t w, const uint16_t h);
        [[nodiscard]] Version version() const noexcept;
        [[nodiscard]] const char* name() const noexcept;

	private:
		std::unique_ptr<Game> _game;
	};

#ifdef J4F_NO_APPLICATION
    class Game {};
	Application::Application() = default;
    Application::~Application() = default;
    void Application::requestFeatures() {}
    void Application::onEngineInitComplete() {}
	void Application::update(const float delta) {}
	void Application::render(const float delta) {}
	void Application::resize(const uint16_t w, const uint16_t h) {}
    Version Application::version() const noexcept { return Version(0, 0, 0); }
    const char* Application::getName() const noexcept { return "no_application"; }
#endif
}