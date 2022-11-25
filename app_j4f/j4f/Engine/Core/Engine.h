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
	class WorkerThread;
	
	class Engine {
		friend class std::thread;
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

		inline void setGameTimeMultiply(const float m) { _gameTimeMultiply = m; }
		inline float getGameTimeMultiply() const { return _gameTimeMultiply; }

		inline float getFrameDeltaTime() const { return _frameDeltaTime; }

	private:
		Engine();
		void initComplete();

		void update();

		std::vector<IEngineModule*> _modules;

		uint16_t _frameId = 0;
		Statistic* _statistic = nullptr;
		Graphics* _graphics = nullptr;
		Application* _application = nullptr;
		Looper* _looper = nullptr;
		std::unique_ptr<WorkerThread> _renderThread = nullptr;
		std::unique_ptr<WorkerThread> _updateThread = nullptr;

		std::chrono::steady_clock::time_point _time;
		float _frameDeltaTime = 0.0f;
		double _minframeLimit = std::numeric_limits<double>::max();
		FpsLimitType _frameLimitType = FpsLimitType::F_DONT_CARE;
		float _gameTimeMultiply = 1.0f;
	};
}