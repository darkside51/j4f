#pragma once

#include "Common.h"
#include "Configs.h"
#include "Version.h"

#include "Threads/Task2.h"
#include <deque>

#include <array>
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
        enum class Workers: uint8_t {
            RENDER_THREAD = 0,
            UPDATE_THREAD = 1,
            MAX_VALUE
        };

		~Engine();

		inline static Engine& getInstance() noexcept {
			static Engine engine;
			return engine;
		}

		void init(const EngineConfig& config);
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

		inline void setTimeMultiply(const float m) { _timeMultiply = m; }
		[[nodiscard]] inline float getTimeMultiply() const noexcept { return _timeMultiply; }

        [[nodiscard]] inline static Version version() noexcept { return {0, 0, 1}; }
        [[nodiscard]] Version applicationVersion() const noexcept;

        [[nodiscard]] uint8_t getThreadCommutationId(const Workers w) const noexcept {
            if (w == Workers::MAX_VALUE) { return 0xffffu; }
            return _workerIds[static_cast<uint8_t>(w)];
        }

        [[nodiscard]] uint8_t getThreadCommutationId(const uint8_t id) const noexcept {
            if (id >= static_cast<uint8_t>(Workers::MAX_VALUE)) { return 0xffffu; }
            return _workerIds[id];
        }

	private:
		Engine();
		void initComplete();

		void render(const float delta, const std::chrono::steady_clock::time_point& currentTime, std::deque<linked_ptr<TaskBase>>&& tasks);
		void update(const float delta, const std::chrono::steady_clock::time_point& currentTime, std::deque<linked_ptr<TaskBase>>&& tasks);

		std::vector<IEngineModule*> _modules;

		Statistic* _statistic = nullptr;
		Graphics* _graphics = nullptr;
		Application* _application = nullptr;
		Looper* _looper = nullptr;
		std::unique_ptr<WorkerThread> _renderThread;
		std::unique_ptr<WorkerThread> _updateThread;

		float _timeMultiply = 1.0f;

        std::array<uint8_t, static_cast<size_t>(Workers::MAX_VALUE)> _workerIds;
	};
}