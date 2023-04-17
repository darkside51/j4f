#include "Engine.h"

#include "Application.h"

#include <Platform_inc.h>
#include "Cache.h"
#include "Threads/ThreadPool.h"
#include "Threads/ThreadPool2.h"
#include "Threads/Worker.h"
#include "Threads/WorkersCommutator.h"
#include "Memory/MemoryManager.h"
#include "AssetManager.h"
#include "../File/FileManager.h"
#include "../Graphics/Graphics.h"
#include "../Utils/Statistic.h"
#include "../Utils/Json/Json.h"
#include "../Input/Input.h"
#include "../Events/Bus.h"
//#include "Threads/Looper.h"

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
	
	void Engine::init(const EngineConfig& config) {
		initPlatform();

		setModule<LogManager>();
		setModule<Statistic>();

		//setModule<ThreadPool>(std::max(static_cast<uint8_t>(std::thread::hardware_concurrency()), uint8_t(1)));
		setModule<ThreadPool2>(std::max(static_cast<uint8_t>(std::thread::hardware_concurrency()), uint8_t(1)));
        setModule<WorkerThreadsCommutator>();
		setModule<MemoryManager>();
		setModule<CacheManager>();
		setModule<FileManager>();
		setModule<AssetManager>(2);
		setModule<Input>();
        setModule<Graphics>(config.graphicsCfg);
		setModule<Device>();
		setModule<Bus>();
		
		_statistic = getModule<Statistic>();
		_graphics = getModule<Graphics>();

		getModule<FileManager>()->createFileSystem<DefaultFileSystem>();
		getModule<AssetManager>()->setLoader<JsonLoader>();

        // create application
        _application = std::make_unique<Application>();
        _application->requestFeatures();

        _renderThread = std::make_unique<WorkerThread>(&Engine::render, this);
        _renderThread->setTargetFrameTime(1.0f / config.fpsLimitDraw.fpsMax);
        _renderThread->setFpsLimitType(config.fpsLimitDraw.limitType);

        _updateThread = std::make_unique<WorkerThread>(&Engine::update, this);
        _updateThread->setTargetFrameTime(1.0f / config.fpsLimitUpdate.fpsMax);
        _updateThread->setFpsLimitType(config.fpsLimitUpdate.limitType);

        auto workersCommutator = getModule<WorkerThreadsCommutator>();
        _workerIds[static_cast<uint8_t>(Workers::RENDER_THREAD)] = workersCommutator->emplaceWorkerThread(_renderThread.get());
        _workerIds[static_cast<uint8_t>(Workers::UPDATE_THREAD)] = workersCommutator->emplaceWorkerThread(_updateThread.get());

		initComplete(); // after all

		// run
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
			_application = nullptr;
		}

		deinitPlatform();
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

    inline void executeTaskCollection(std::deque<linked_ptr<TaskBase>>&& tasks) {
        while (!tasks.empty()) {
            if (tasks.front()->state() == TaskState::IDLE) {
                tasks.front()->operator()();
            }
            tasks.pop_front();
        }
    }

	void Engine::update(const float delta,
                        const std::chrono::steady_clock::time_point& /*currentTime*/,
                        std::deque<linked_ptr<TaskBase>>&& tasks) {
        if (_application) {
            _application->update(delta);
        }

        executeTaskCollection(std::move(tasks));

        if (_statistic) {
            _statistic->update(delta);
        }
	}

	void Engine::render(const float delta,
                           const std::chrono::steady_clock::time_point& currentTime,
                           std::deque<linked_ptr<TaskBase>>&& tasks) {
		_graphics->beginFrame();

        if (_application) {
            _application->render(delta);
        }

        executeTaskCollection(std::move(tasks));

		if (_statistic) {
            _statistic->render(delta);
			_statistic->frame(delta);
			_statistic->addFramePrepareTime((std::chrono::duration<float>(std::chrono::steady_clock::now() - currentTime)).count());
		}

		_graphics->endFrame();
	}

    Version Engine::applicationVersion() const noexcept {
        if (!_application) return {0, 0, 0};
        return _application->version();
    }
}