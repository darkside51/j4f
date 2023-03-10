#pragma once

#include "Common.h"
#include "Configs.h"
#include "Version.h"

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
        T* replaceModule(T* module) {
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

		void resize(const uint16_t w, const uint16_t h);
		void deviceDestroyed();

		void run();

		inline void setGameTimeMultiply(const float m) { _gameTimeMultiply = m; }
		[[nodiscard]] inline float getGameTimeMultiply() const noexcept { return _gameTimeMultiply; }

        [[nodiscard]] inline static Version version() noexcept { return {0, 0, 1}; }
        [[nodiscard]] Version applicationVersion() const noexcept;
	private:
		Engine();
		void initComplete();

		void nextFrame(const float delta, const std::chrono::steady_clock::time_point& currentTime);
		void update(const float delta, const std::chrono::steady_clock::time_point& currentTime);

		std::vector<IEngineModule*> _modules;

		Statistic* _statistic = nullptr;
		Graphics* _graphics = nullptr;
		Application* _application = nullptr;
		Looper* _looper = nullptr;
		std::unique_ptr<WorkerThread> _renderThread;
		std::unique_ptr<WorkerThread> _updateThread;

		float _gameTimeMultiply = 1.0f;
	};
}