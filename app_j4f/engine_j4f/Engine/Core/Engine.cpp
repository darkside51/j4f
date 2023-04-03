#include "Engine.h"

#include "Application.h"

#include <Platform_inc.h>
#include "Cache.h"
#include "Threads/ThreadPool.h"
#include "Threads/ThreadPool2.h"
#include "Threads/Worker.h"
#include "Threads/Looper.h"
#include "Memory/MemoryManager.h"
#include "AssetManager.h"
#include "../File/FileManager.h"
#include "../Graphics/Graphics.h"
#include "../Utils/Statistic.h"
#include "../Utils/Json/Json.h"
#include "../Input/Input.h"
#include "../Events/Bus.h"

#include <cstdint>
#include <chrono>

namespace engine {

	Engine::Engine() : _renderThread(nullptr), _updateThread(nullptr) {
#ifdef _DEBUG
		enableMemoryLeaksDebugger();
#endif
	}

	Engine::~Engine() {
		destroy();
	}
	
	void Engine::init(const EngineConfig& cfg) {
		setModule<LogManager>();
		setModule<Statistic>();

		//setModule<ThreadPool>(std::max(static_cast<uint8_t>(std::thread::hardware_concurrency()), uint8_t(1)));
		setModule<ThreadPool2>(std::max(static_cast<uint8_t>(std::thread::hardware_concurrency()), uint8_t(1)));
		setModule<Looper>();
		setModule<MemoryManager>();
		setModule<CacheManager>();
		setModule<FileManager>();
		setModule<AssetManager>(2);
		setModule<Input>();
        setModule<Graphics>(cfg.graphicsCfg);
		setModule<Device>();
		setModule<Bus>();
		
		_statistic = getModule<Statistic>();
		_graphics = getModule<Graphics>();
		_looper = getModule<Looper>();

		getModule<FileManager>()->createFileSystem<DefaultFileSystem>();
		getModule<AssetManager>()->setLoader<JsonLoader>();

        // create application
        _application = new Application();
        _application->requestFeatures();

		initComplete(); // after all

		// run
		_renderThread = std::make_unique<WorkerThread>(&Engine::nextFrame, this);
		_renderThread->setTargetFrameTime(1.0f / cfg.fpsDraw);
		_renderThread->setFpsLimitType(cfg.fpsLimitTypeDraw);

		_updateThread = std::make_unique<WorkerThread>(&Engine::update, this);
        _updateThread->setTargetFrameTime(1.0f / cfg.fpsUpdate);
        _updateThread->setFpsLimitType(cfg.fpsLimitTypeUpdate);

		run();
	}

	void Engine::initComplete() {
        getModule<Device>()->setTittle(_application->getName());
		_graphics->onEngineInitComplete();
        _application->onEngineInitComplete();
	}

	void Engine::run() {
		if (_renderThread) {
			_renderThread->run();
		}

		if (_updateThread) {
			_updateThread->run();
		}

		getModule<Device>()->start();
	}

	void Engine::destroy() {
		_renderThread = nullptr;
		_updateThread = nullptr;

		for (auto&& m : _modules) {
			delete m;
		}
		_modules.clear();

		if (_application) {
			delete _application;
			_application = nullptr;
		}
	}

	void Engine::resize(const uint16_t w, const uint16_t h) {
		if (_renderThread) {
			if (_renderThread->isActive()) {
				_renderThread->pause();

				_graphics->resize(w, h);

                if (_application) {
                    _application->resize(w, h);
                }

				if (w > 0 && h > 0) {
					_renderThread->resume();
					getModule<ThreadPool2>()->resume();
				} else {
					getModule<ThreadPool2>()->pause();
				}
			} else {
				if (w > 0 && h > 0) {
					_graphics->resize(w, h);

                    if (_application) {
                        _application->resize(w, h);
                    }

					_renderThread->resume();
					getModule<ThreadPool2>()->resume();
				} else {
					getModule<ThreadPool2>()->pause();
				}
			}
		} else {
			_graphics->resize(w, h);

            if (_application) {
                _application->resize(w, h);
            }

			if (w > 0 && h > 0) {
				getModule<ThreadPool2>()->resume();
			} else {
				getModule<ThreadPool2>()->pause();
			}
		}
	}

	void Engine::deviceDestroyed() {
		if (_renderThread) {
			_renderThread->stop();
		}

		if (_updateThread) {
			_updateThread->stop();
		}

		getModule<ThreadPool2>()->stop();
		getModule<AssetManager>()->deviceDestroyed();
		_graphics->deviceDestroyed();

        if (_application) {
            _application->deviceDestroyed();
        }
	}

	void Engine::update(const float delta, const std::chrono::steady_clock::time_point& /*currentTime*/) {
        if (_application) {
            _application->update(delta);
        }
	}

	void Engine::nextFrame(const float delta, const std::chrono::steady_clock::time_point& currentTime) {
		_graphics->beginFrame();

        if (_application) {
            _application->nextFrame(delta);
        }

		_looper->nextFrame(delta);

		if (_statistic) {
			_statistic->nextFrame(delta);
			_statistic->addFramePrepareTime(static_cast<float>((std::chrono::duration<double>(std::chrono::steady_clock::now() - currentTime)).count()));
		}

		_graphics->endFrame();
	}

    Version Engine::applicationVersion() const noexcept {
        if (!_application) return {0, 0, 0};
        return _application->version();
    }
}