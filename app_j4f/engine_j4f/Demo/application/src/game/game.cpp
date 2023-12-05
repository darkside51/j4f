#include "game.h"
#include "graphics/scene.h"

#include <Engine/File/FileManager.h>
#include <Engine/Graphics/UI/ImGui/Imgui.h>
#include <Platform_inc.h>

namespace engine {

	Game::Game() : _scene(nullptr) {
        Engine::getInstance().getModule<Input>().addObserver(this);
    }
	Game::~Game() {
        Engine::getInstance().getModule<Input>().removeObserver(this);
    }

	void Game::onEngineInitComplete() {
        FileManager& fm = Engine::getInstance().getModule<FileManager>();
        auto&& fs = fm.getFileSystem<DefaultFileSystem>();
        fm.mapFileSystem(fs);

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
        if (_scene) {
            _scene->resize(w, h);
        }
	}

    bool Game::onInputPointerEvent(const PointerEvent& event) {
        if (_scene && _scene->getUiGraphics()->onInputPointerEvent(event)) {
            return true;
        }
        return false;
    }

    bool Game::onInputWheelEvent(const float dx, const float dy) {
        if (_scene && _scene->getUiGraphics()->onInputWheelEvent(dx, dy)) {
            return true;
        }
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

        if (_scene && _scene->getUiGraphics()->onInpuKeyEvent(event)) {
            return true;
        }

        return false;
    }

    bool Game::onInpuCharEvent(const uint16_t code) { return false; }

}