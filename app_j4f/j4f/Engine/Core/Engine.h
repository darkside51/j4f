#pragma once

#include "Common.h"
#include "Configs.h"

#include <thread>
#include <atomic>
#include <memory>
#include <functional>

#include <vector>
#include <cstdint>
#include <chrono>

namespace engine {
	class IEngineModule;
	class Graphics;
	class Statistic;
	class Application;
	class Looper;

	class Engine;

	class WorkerThread {
		friend class Engine;
	public:

		template <typename T, typename F, typename... Args>
		WorkerThread(F T::* f, T* t, Args&&... args) {
			_task = [f, t, args...]() { (t->*f)(std::forward<Args>(args)...); };
			_thread = std::thread(&WorkerThread::work, this);
		}

		template <typename F, typename... Args>
		WorkerThread(F&& f, Args&&... args) {
			_task = [f, args...]() { f(std::forward<Args>(args)...); };
			_thread = std::thread(&WorkerThread::work, this);
		}

		~WorkerThread() {
			stop();
		}

		inline void work() {
			while (isAlive()) {
				_wait.clear();

				while (isActive()) {
					_task();
				}

				_wait.test_and_set();

				if (isAlive()) {
					std::unique_lock<std::mutex> lock(_mutex);
					_condition.wait(lock, [this] { return isActive(); });
				}
			}
		}

		inline bool isActive() const { return !_paused.test(std::memory_order_acquire); }
		inline bool isAlive() const { return !_stop.test(std::memory_order_acquire); }

		inline void pause() { _paused.test_and_set(std::memory_order_release); }
		inline void resume() { _paused.clear(); }

		inline void wait() {
			while (!_wait.test()) {
				std::this_thread::yield();
			}
		}

		inline void notify() { _condition.notify_one(); }

		inline void stop() {
			if (!_stop.test_and_set(std::memory_order_release)) {
				_paused.test_and_set(std::memory_order_release);
				if (_thread.joinable()) {
					_thread.join();
				}
			}
		}

	private:
		std::thread _thread;
		std::atomic_flag _paused;
		std::atomic_flag _stop;
		std::atomic_flag _wait;

		std::mutex _mutex;
		std::condition_variable _condition;
		std::function<void()> _task;
	};

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