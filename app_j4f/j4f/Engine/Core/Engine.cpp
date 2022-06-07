#include "Engine.h"

#include "Application.h"
#include "EngineModule.h"

#include <Platform_inc.h>
#include "Cache.h"
#include "Threads/ThreadPool.h"
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
		setModule<Statistic>();

		setModule<ThreadPool>(std::max(static_cast<uint8_t>(std::thread::hardware_concurrency()), uint8_t(1)));
		setModule<Looper>();
		setModule<MemoryManager>();
		setModule<CacheManager>();
		setModule<FileManager>();
		setModule<AssetManager>(2);
		setModule<Input>();
		setModule<Device>();
		setModule<Graphics>(GraphicConfig{true, 2}); // 1 - VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, 2 - VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
		setModule<Bus>();
		
		_statistic = getModule<Statistic>();
		_graphics = getModule<Graphics>();
		_looper = getModule<Looper>();

		getModule<FileManager>()->createFileSystem<DefaultFileSystem>();
		getModule<AssetManager>()->setLoader<JsonLoader>();

		initComplete(); // after all

		// create application and run
		_application = new Application();
		run();
	}

	void Engine::initComplete() {
		_graphics->onEngineInitComplete();
	}

	void Engine::run() {
		getModule<Device>()->start();
	}

	void Engine::destroy() {
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
		_graphics->resize(w, h);
		_application->resize(w, h);
	}

	void Engine::deviceDestroyed() {
		getModule<ThreadPool>()->stop();
		getModule<AssetManager>()->deviceDestroyed();
		_graphics->deviceDestroyed();
		_application->deviceDestroyed();
	}

	void Engine::nextFrame() {
		static auto time = std::chrono::steady_clock::now();
		const auto currentTime = std::chrono::steady_clock::now();
		const std::chrono::duration<double> diff = currentTime - time;
		time = currentTime;

		const float delta = static_cast<float>(diff.count());

		_graphics->beginFrame();
		_application->nextFrame(delta);

		_looper->nextFrame(delta);

		if (_statistic) {
			_statistic->nextFrame(delta);
			_statistic->addFramePrepareTime(static_cast<float>((std::chrono::duration<double>(std::chrono::steady_clock::now() - currentTime)).count()));
		}

		_graphics->endFrame();

		++_frameId; // increase frameId at the end of frame
	}
}