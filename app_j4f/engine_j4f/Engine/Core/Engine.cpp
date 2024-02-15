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
#include "../Time/TimerManager.h"

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

#ifdef ENABLE_STATISTIC
		setModule<Statistic>(1.0f);
#endif

		//setModule<ThreadPool>(std::max(std::thread::hardware_concurrency(), 1u));
		//setModule<ThreadPool2>("main_pool", std::max(std::thread::hardware_concurrency(), 1u));
		setModule<ThreadPool2>("main_pool", std::min(4u, std::max(std::thread::hardware_concurrency(), 1u)));
        setModule<WorkerThreadsCommutator>();
		setModule<MemoryManager>();
		setModule<CacheManager>();
		setModule<FileManager>();
		setModule<AssetManager>(2u);
		setModule<Input>();
        setModule<Graphics>(config.graphicsCfg);
		setModule<Device>();
		setModule<Bus>();
        setModule<TimerManager>();

		_statistic = hasModule<Statistic>() ? &getModule<Statistic>() : nullptr;
		_graphics = &getModule<Graphics>();

		getModule<FileManager>().createFileSystem<DefaultFileSystem>();
		getModule<AssetManager>().setLoader<JsonLoader>();

        // create application
        _application = std::make_unique<Application>();
        _application->requestFeatures();

        // workers
        _renderThread = std::make_unique<WorkerThread>(&Engine::render, this);
        _renderThread->setTargetFrameTime(1.0f / static_cast<float>(config.fpsLimitDraw.fpsMax));
        _renderThread->setFpsLimitType(config.fpsLimitDraw.limitType);

        _updateThread = std::make_unique<WorkerThread>(&Engine::update, this);
        _updateThread->setTargetFrameTime(1.0f / static_cast<float>(config.fpsLimitUpdate.fpsMax));
        _updateThread->setFpsLimitType(config.fpsLimitUpdate.limitType);

        auto & workersCommutator = getModule<WorkerThreadsCommutator>();
        _workerIds[static_cast<uint8_t>(Workers::RENDER_THREAD)] = workersCommutator.emplaceWorkerThread(_renderThread.get());
        _workerIds[static_cast<uint8_t>(Workers::UPDATE_THREAD)] = workersCommutator.emplaceWorkerThread(_updateThread.get());

		initComplete(); // after all and before run
		run(); // start work
	}

	void Engine::initComplete() {
        getModule<Device>().setTittle(_application->name());
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

		getModule<Device>().start();
	}

	void Engine::destroy() {
		_renderThread = nullptr;
		_updateThread = nullptr;

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

				if (w > 0u && h > 0u) {
					_renderThread->resume();
					getModule<ThreadPool2>().resume();
				} else {
					getModule<ThreadPool2>().pause();
				}
			} else {
				if (w > 0u && h > 0u) {
					_graphics->resize(w, h);

                    if (_application) {
                        _application->resize(w, h);
                    }

					_renderThread->resume();
					getModule<ThreadPool2>().resume();
				} else {
					getModule<ThreadPool2>().pause();
				}
			}
		} else {
			_graphics->resize(w, h);

            if (_application) {
                _application->resize(w, h);
            }

			if (w > 0u && h > 0u) {
				getModule<ThreadPool2>().resume();
			} else {
				getModule<ThreadPool2>().pause();
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

		getModule<ThreadPool2>().stop();
		getModule<AssetManager>().deviceDestroyed();
		_graphics->deviceDestroyed();

        if (_application) {
            _application.reset();
        }
	}

    inline void executeTaskCollection(WorkerTasks && tasks) {
        while (!tasks.empty()) {
            tasks.front()();
            tasks.pop_front();
        }
    }

	void Engine::update(const float delta,
                        const std::chrono::steady_clock::time_point& /*currentTime*/,
                        WorkerTasks && tasks) {
		executeTaskCollection(std::move(tasks));

        if (_application) {
            _application->update(delta);
        }

#ifdef ENABLE_STATISTIC
        if (_statistic) {
            _statistic->update(delta);
        }
#endif
	}

	void Engine::render(const float delta,
                           const std::chrono::steady_clock::time_point& currentTime,
                           WorkerTasks && tasks) {
		_graphics->beginFrame();

		executeTaskCollection(std::move(tasks));

        if (_application) {
            _application->render(delta);
        }

#ifdef ENABLE_STATISTIC
		if (_statistic) {
            _statistic->render(delta);
			_statistic->frame(delta);
			_statistic->addFramePrepareTime((std::chrono::duration<float>(std::chrono::steady_clock::now() - currentTime)).count());
		}
#endif

		_graphics->endFrame();
	}

    Version Engine::applicationVersion() const noexcept {
        if (!_application) return {0u, 0u, 0u};
        return _application->version();
    }
}