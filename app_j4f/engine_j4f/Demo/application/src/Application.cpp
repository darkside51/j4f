#include <Engine/Core/Application.h>
#include <Engine/Core/Engine.h>
#include <Engine/Graphics/Graphics.h>
#include <Engine/Graphics/Features/Shadows/CascadeShadowMap.h>


#include "game/game.h"

namespace engine {
	
Application::Application() : _game(std::make_unique<Game>()) {}
Application::~Application() {}

void Application::requestFeatures() {
    Engine::getInstance().getModule<Graphics>().features().request<CascadeShadowMap>(ShadowMapTechnique::SMT_INSTANCE_DRAW);
}

void Application::onEngineInitComplete() {
	_game->onEngineInitComplete();
}

void Application::update(const float delta) {
	_game->update(delta);
}

void Application::render(const float delta) {
	_game->render(delta);
}

void Application::resize(const uint16_t w, const uint16_t h) {
	_game->resize(w, h);
}

Version Application::version() const noexcept { return Version(0, 0, 0); }

const char* Application::name() const noexcept { return "demo"; }

}