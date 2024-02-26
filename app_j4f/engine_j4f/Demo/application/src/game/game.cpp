#include "game.h"
#include "graphics/graphics_factory.h"
#include "graphics/scene.h"
#include "world.h"

#include <Engine/File/FileManager.h>
#include <Engine/Graphics/UI/ImGui/Imgui.h>
#include <Platform_inc.h>

#include "service_locator.h"

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

        _graphicsFactory = std::make_unique<game::GraphicsFactory>();
        game::ServiceLocator::instance().registerService<game::GraphicsFactory>(_graphicsFactory);

        game::ServiceLocator::instance().registerService<game::PlayerController>(&_controller.getPlayerController());

        _scene = std::make_unique<game::Scene>();
        game::ServiceLocator::instance().registerService<game::Scene>(_scene);

        _world = std::make_unique<game::World>();
        game::ServiceLocator::instance().registerService<game::World>(_world);

        auto & cameraController = _controller.getCameraController();
        _scene->assignCameraController(engine::make_ref(cameraController));

        initialise();
	}

    void Game::initialise() {
        _graphicsFactory->loadObjects(std::string_view{"resources/assets/configs/graphics.json"});
        _world->create();
    }

	void Game::update(const float delta) {
        if (_scene) {
            _scene->update(delta);
            _world->update(delta);
        }
	}

	void Game::render(const float delta) {
        _controller.onRenderFrame();

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
        return _controller.onInputPointerEvent(event);
    }

    bool Game::onInputWheelEvent(const float dx, const float dy) {
        if (_scene && _scene->getUiGraphics()->onInputWheelEvent(dx, dy)) {
            return true;
        }
        return _controller.onInputWheelEvent(dx, dy);
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
                return true;
            }
            break;
        default:
            break;
        }

        if (_scene && _scene->getUiGraphics()->onInpuKeyEvent(event)) {
            return true;
        }

        return _controller.onInpuKeyEvent(event);
    }

    bool Game::onInpuCharEvent(const uint16_t code) { return _controller.onInpuCharEvent(code); }

}