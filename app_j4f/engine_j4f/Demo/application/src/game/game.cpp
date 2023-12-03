#include "game.h"
#include "graphics/scene.h"

#include <Platform_inc.h>

namespace engine {

	Game::Game() {
        Engine::getInstance().getModule<Input>().addObserver(this);
    }
	Game::~Game() {
        Engine::getInstance().getModule<Input>().removeObserver(this);
    }

	void Game::onEngineInitComplete() {
        _scene = std::make_unique<game::Scene>();
	}

	void Game::update(const float delta) {
        if (_scene) {
            _scene->update(delta);
        }
	}

	void Game::render(const float delta) {
        if (_scene) {
            _scene->render(delta);
        }
    }

	void Game::resize(const uint16_t w, const uint16_t h) {
	}

    bool Game::onInputPointerEvent(const PointerEvent& event) {
        return false;
    }

    bool Game::onInputWheelEvent(const float dx, const float dy) {
        return false;
    }

    bool Game::onInpuKeyEvent(const KeyEvent& event) {
        switch (event.key) {
        case KeyboardKey::K_ESCAPE:
            if (event.state != InputEventState::IES_RELEASE) break;
            Engine::getInstance().getModule<Device>().leaveMainLoop();
            return true;
        case KeyboardKey::K_ENTER:
            if (Engine::getInstance().getModule<Input>().isAltPressed()) {
                if (event.state != InputEventState::IES_RELEASE) break;
                Engine::getInstance().getModule<Device>().setFullscreen(!Engine::getInstance().getModule<Device>().isFullscreen());
            }
            return true;
        default:
            break;
        }

        return false;
    }

    bool Game::onInpuCharEvent(const uint16_t code) { return false; }

}