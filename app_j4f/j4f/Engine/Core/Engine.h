#pragma once

#include "Common.h"
#include "Configs.h"

#include <vector>
#include <cstdint>
#include <chrono>

namespace engine {
	class IEngineModule;
	class Graphics;
	class Statistic;
	class Application;
	class Looper;

	class Engine {
	public:
		~Engine();

		inline static Engine& getInstance() noexcept {
			static Engine engine;
			return engine;
		}

		void init(const EngineConfig& cfg);
		void destroy();

		template<typename T>
		inline T* getModule() { 
			const uint16_t moduleId = UniqueTypeId<IEngineModule>::getUniqueId<T>();
			if (moduleId < _modules.size()) {
				return static_cast<T*>(_modules[moduleId]);
			}

			return nullptr;
		}

		template<typename T>
		inline const T* getModule() const { 
			const uint16_t moduleId = UniqueTypeId<IEngineModule>::getUniqueId<T>();
			if (moduleId < _modules.size()) {
				return static_cast<T*>(_modules[moduleId]);
			}

			return nullptr;
		}

		template<typename T>
		T* setModule(T* module) {
			const uint16_t moduleId = UniqueTypeId<IEngineModule>::getUniqueId<T>();
			T* oldModule = nullptr;
			if (_modules.size() <= moduleId) {
				_modules.resize(moduleId + 1);
			} else {
				oldModule = static_cast<T*>(_modules[moduleId]);
			}
			
			_modules[moduleId] = module;
			return oldModule;
		}

		template<typename T, typename...Args>
		T* setModule(Args&&...args) {
			const uint16_t moduleId = UniqueTypeId<IEngineModule>::getUniqueId<T>();
			T* oldModule = nullptr;
			if (_modules.size() <= moduleId) {
				_modules.resize(moduleId + 1);
			} else {
				oldModule = static_cast<T*>(_modules[moduleId]);
			}

			_modules[moduleId] = new T(std::forward<Args>(args)...);
			return oldModule;
		}

		inline uint16_t getFrameId() const { return _frameId; }
		void nextFrame();
		void resize(const uint16_t w, const uint16_t h);
		void deviceDestroyed();

		void run();

	private:
		Engine();
		void initComplete();

		std::vector<IEngineModule*> _modules;

		uint16_t _frameId = 0;
		Statistic* _statistic = nullptr;
		Graphics* _graphics = nullptr;
		Application* _application = nullptr;
		Looper* _looper = nullptr;

		std::chrono::steady_clock::time_point _time;
		double _minframeLimit = std::numeric_limits<double>::max();
	};
}