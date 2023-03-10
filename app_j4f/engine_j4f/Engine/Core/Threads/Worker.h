#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <functional>
#include <chrono>
#include <condition_variable>

#include "../../Log/Log.h"
#include "Synchronisations.h"
#include "../Configs.h"

namespace engine {

	class WorkerThread {
	public:
		template <typename T, typename F, typename... Args>
		explicit WorkerThread(F T::* f, T* t, Args&&... args) : 
			_task([f, t, args...](const float time, const std::chrono::steady_clock::time_point& currentTime) { (t->*f)(time, currentTime, std::forward<Args>(args)...); }),
			_time(std::chrono::steady_clock::now()) {
		}

		template <typename F, typename... Args>
		explicit WorkerThread(F&& f, Args&&... args) :
			_task([f, args...](const float time, const std::chrono::steady_clock::time_point& currentTime) { f(time, currentTime, std::forward<Args>(args)...); }),
			_time(std::chrono::steady_clock::now()) {
		}

		~WorkerThread() {
			stop();
		}

		WorkerThread(const WorkerThread&) = delete;
		const WorkerThread& operator= (WorkerThread&) = delete;

		inline void run() {
			_thread = std::thread(&WorkerThread::work, this);
		}

		inline void work() {
			while (isAlive()) {
				_wait.clear(std::memory_order_release);

				while (isActive()) {
					const auto currentTime = std::chrono::steady_clock::now();
					const std::chrono::duration<float> duration = currentTime - _time;
					const float durationTime = duration.count();
					
					switch (_fpsLimitType) {
						case FpsLimitType::F_STRICT:
							if (durationTime < _targetFrameTime) {
								std::this_thread::yield();
								continue;
							}
							break;
						case FpsLimitType::F_CPU_SLEEP:
							if (durationTime < _targetFrameTime) {
								std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(1000.0f * (_targetFrameTime - durationTime)));
                                continue;
							}
							break;
						default:
							break;
					}

					_time = currentTime;
					_task(durationTime, currentTime);

					_frameId.fetch_add(1, std::memory_order_release); // increase frameId at the end of frame
					//std::this_thread::yield();
				}

				_wait.test_and_set(std::memory_order_acq_rel);
				//LOG_TAG_LEVEL(engine::LogLevel::L_DEBUG, THREAD, "pause worker thread");

				if (isAlive()) {
					std::function<bool()> pauseCallback;
					{
						AtomicLockF lock(_callbackLock);
						pauseCallback = std::move(_onPause);
						_onPause = nullptr;
					}

					if (pauseCallback) {
						if (pauseCallback()) {
							sleep();
						} else {
							requestResume(); // remove pause flag if _onPause returned false
						}
					} else {
						sleep();
					}
				}

				//LOG_TAG_LEVEL(engine::LogLevel::L_DEBUG, THREAD, "resume worker thread");
				std::this_thread::yield();
			}
		}

		[[nodiscard]] inline bool isActive() const noexcept { return !_paused.test(std::memory_order_acquire); }
        [[nodiscard]] inline bool isAlive() const noexcept { return !_stop.test(std::memory_order_acquire); }

		inline void resume() {
			requestResume();
			notify();
		}

		inline void pause() {
			requestPause(nullptr);
			waitPaused();
		}

		inline void stop() {
			if (!_stop.test_and_set(std::memory_order_acq_rel)) {
				_paused.test_and_set(std::memory_order_release);
				if (_thread.joinable()) {
					_thread.join();
				}
			}
		}

		inline void requestPause(const std::function<bool()>& onPause) { 
			{
				AtomicLockF lock(_callbackLock);
				_onPause = onPause;
			}
			_paused.test_and_set(std::memory_order_release);
		}

		inline void requestPause(std::function<bool()>&& onPause) {
			{
				AtomicLockF lock(_callbackLock);
				_onPause = std::move(onPause);
			}
			_paused.test_and_set(std::memory_order_release);
		}

		inline void requestResume() {
			_paused.clear(std::memory_order_release);
		}

		inline void waitPaused() {
			while (!_wait.test(std::memory_order_acquire)) {
				std::this_thread::yield();
			}
		}

		void setTargetFrameTime(const double t) {
			_targetFrameTime = t;
		}

		void setFpsLimitType(const FpsLimitType t) {
			_fpsLimitType = t;
		}

		[[nodiscard]] inline uint16_t getFrameId() const noexcept { return _frameId.load(std::memory_order_consume); }

	private:
		inline void sleep() {
			std::unique_lock<std::mutex> lock(_mutex);
			_condition.wait(lock, [this] { return isActive(); });
		}

		inline void notify() { _condition.notify_one(); }

		std::thread _thread;
		std::atomic_flag _paused;
		std::atomic_flag _stop;
		std::atomic_flag _wait;

		std::mutex _mutex;
		std::condition_variable _condition;

		std::function<void(const float, const std::chrono::steady_clock::time_point&)> _task = nullptr;

		std::atomic_flag _callbackLock;
		std::function<bool()> _onPause = nullptr;

		std::chrono::steady_clock::time_point _time;
		std::atomic_uint16_t _frameId = { 0 };

		float _targetFrameTime = std::numeric_limits<float>::max();
		FpsLimitType _fpsLimitType = FpsLimitType::F_DONT_CARE;
	};

}
