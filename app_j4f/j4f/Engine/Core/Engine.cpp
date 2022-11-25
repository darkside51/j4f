#include "Engine.h"

#include "Application.h"
#include "EngineModule.h"

#include <Platform_inc.h>
#include "Cache.h"
#include "Threads/ThreadPool.h"
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
#include <algorithm>

namespace engine {

	Engine::Engine() {
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

		setModule<ThreadPool>(std::max(static_cast<uint8_t>(std::thread::hardware_concurrency()), uint8_t(1)));
		setModule<Looper>();
		setModule<MemoryManager>();
		setModule<CacheManager>();
		setModule<FileManager>();
		setModule<AssetManager>(2);
		setModule<Input>();
		setModule<Device>();
		setModule<Graphics>(cfg.graphicsCfg);
		setModule<Bus>();
		
		_statistic = getModule<Statistic>();
		_graphics = getModule<Graphics>();
		_looper = getModule<Looper>();

		getModule<FileManager>()->createFileSystem<DefaultFileSystem>();
		getModule<AssetManager>()->setLoader<JsonLoader>();

		_minframeLimit = 1.0 / cfg.fpsLimit;
		_frameLimitType = cfg.fpsLimitType;
		_time = std::chrono::steady_clock::now();

		initComplete(); // after all

		// create application and run
		_application = new Application();
		run();
	}

	void Engine::initComplete() {
		_graphics->onEngineInitComplete();
	}

	void Engine::run() {
		//_updateThread = new WorkerThread(&Engine::update, this);
		_renderThread = std::make_unique<WorkerThread>(&Engine::nextFrame, this);

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
				_application->resize(w, h);

				_renderThread->resume();
			}
		} else {
			_graphics->resize(w, h);
			_application->resize(w, h);
		}
	}

	void Engine::deviceDestroyed() {
		if (_renderThread) {
			_renderThread->stop();
		}

		if (_updateThread) {
			_updateThread->stop();
		}

		getModule<ThreadPool>()->stop();
		getModule<AssetManager>()->deviceDestroyed();
		_graphics->deviceDestroyed();
		_application->deviceDestroyed();
	}

	void Engine::update() {
		_application->update(0.0f);
	}

	void Engine::nextFrame() {
		const auto currentTime = std::chrono::steady_clock::now();
		const std::chrono::duration<double> duration = currentTime - _time;
		const double durationTime = duration.count();

		switch (_frameLimitType) {
			case FpsLimitType::F_STRICT:
				if (durationTime < _minframeLimit) {
					return;
				}
				break;
			case FpsLimitType::F_CPU_SLEEP:
				if (durationTime < _minframeLimit) {
					std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(1000.0 * (_minframeLimit - durationTime)));
				}
				break;
			default:
				break;
		}

		_time = currentTime;
		_frameDeltaTime = static_cast<float>(durationTime);

		const auto& size = _graphics->getSize();
		if (size.first != 0 && size.second != 0) {
			_graphics->beginFrame();
			_application->nextFrame(_frameDeltaTime * _gameTimeMultiply);

			_looper->nextFrame(_frameDeltaTime);

			if (_statistic) {
				_statistic->nextFrame(_frameDeltaTime);
				_statistic->addFramePrepareTime(static_cast<float>((std::chrono::duration<double>(std::chrono::steady_clock::now() - currentTime)).count()));
			}

			_graphics->endFrame();
		}

		++_frameId; // increase frameId at the end of frame
	}
}