#pragma once

#include "../../Log/Log.h"
#include "Synchronisations.h"
#include "TaskCommon.h"
#include "../Configs.h"

#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <functional>
#include <chrono>
#include <condition_variable>
#include <optional>

#undef min
#undef max

namespace engine {

	class WorkerThread {
	public:
		template <typename T, typename F, typename... Args>
		explicit WorkerThread(F T::* f, T* t, Args&&... args) :
			_task([f, t, args...](const float time,
                                  const std::chrono::steady_clock::time_point& currentTime,
                                  WorkerTasks && tasks) {
                (t->*f)(time, currentTime, std::move(tasks), std::forward<Args>(args)...);
            }) {
		}

		template <typename F, typename... Args>
		explicit WorkerThread(F&& f, Args&&... args) :
			_task([f = std::forward<F>(f), args...](const float time,
                                                    const std::chrono::steady_clock::time_point& currentTime,
                                                    WorkerTasks && tasks) {
                f(time, currentTime, std::move(tasks), std::forward<Args>(args)...);
            }) {
		}

		~WorkerThread() {
			stop();
		}

		WorkerThread(const WorkerThread&) = delete;
		const WorkerThread& operator= (WorkerThread&) = delete;

		inline void run() {
            _time = std::chrono::steady_clock::now();
			_thread = std::thread(&WorkerThread::work, this);
		}

		inline void work() {
            _threadId = std::this_thread::get_id();

			while (isAlive()) {
				_wait.clear(std::memory_order_relaxed);

				while (isActive()) {
					const auto currentTime = std::chrono::steady_clock::now();
					const std::chrono::duration<double> duration = currentTime - _time; // as default in seconds
					const double durationTime = duration.count();

					switch (_fpsLimitType) {
						case FpsLimitType::F_STRICT:
							if (durationTime < _targetFrameTime) {
								std::this_thread::yield();
								continue;
							}
							break;
						case FpsLimitType::F_CPU_SLEEP:
							if (const double t = (_targetFrameTime - durationTime); t > 0.0) {
								if (_stealedTime <= t) {
									std::this_thread::sleep_for(std::chrono::duration<double>(t)); // as default in seconds
									_stealedTime = std::chrono::duration<double>(std::chrono::steady_clock::now() - currentTime).count() - t;
									continue;
								} else {
									_stealedTime -= t;
								}
							}
							break;
						default:
							break;
					}

					_time = currentTime;

                    WorkerTasks tasks;
                    {
                        std::lock_guard<std::mutex> lock(_tasksMutex);
                        tasks = std::move(_tasks);
                    }

                    _task(static_cast<float>(durationTime), currentTime, std::move(tasks));

					_frameId.fetch_add(1u, std::memory_order_relaxed); // increase frameId at the end of frame
				}

				_wait.test_and_set(std::memory_order_relaxed);
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

		[[nodiscard]] inline bool isActive() const noexcept { return !_paused.test(std::memory_order_relaxed); }
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
				_paused.test_and_set(std::memory_order_relaxed);
				if (_thread.joinable()) {
					_thread.join();
				}
                _threadId = std::nullopt;
			}
		}

		inline void requestPause(const std::function<bool()>& onPause) {
			{
				AtomicLockF lock(_callbackLock);
				_onPause = onPause;
			}
			_paused.test_and_set(std::memory_order_relaxed);
		}

		inline void requestPause(std::function<bool()>&& onPause) {
			{
				AtomicLockF lock(_callbackLock);
				_onPause = std::move(onPause);
			}
			_paused.test_and_set(std::memory_order_relaxed);
		}

		inline void requestResume() {
			_paused.clear(std::memory_order_relaxed);
		}

		inline void waitPaused() {
			while (!_wait.test(std::memory_order_relaxed)) {
				std::this_thread::yield();
			}
		}

		void setTargetFrameTime(const float t) {
			_targetFrameTime = t;
		}

		void setFpsLimitType(const FpsLimitType t) {
			_fpsLimitType = t;
		}

        void emplaceTask(std::function<void()>&& task) {
            std::lock_guard<std::mutex> lock(_tasksMutex);
            _tasks.emplace_back(std::move(task));
            //_tasks.emplace_front(std::move(task));
        }

		[[nodiscard]] inline uint16_t frameId() const noexcept { return _frameId.load(std::memory_order_relaxed); }
        [[nodiscard]] inline const std::optional<std::thread::id>& threadId() const noexcept { return _threadId; }

	private:
		inline void sleep() {
			std::unique_lock<std::mutex> lock(_mutex);
			_condition.wait(lock, [this] { return isActive(); });
		}

		inline void notify() { _condition.notify_one(); }

        std::optional<std::thread::id> _threadId = std::nullopt;
		std::thread _thread;
		std::atomic_flag _paused;
		std::atomic_flag _stop;
		std::atomic_flag _wait;

		std::mutex _mutex;
		std::condition_variable _condition;

        std::function<void(const float, const std::chrono::steady_clock::time_point&, WorkerTasks&&)> _task = nullptr;

		std::atomic_flag _callbackLock;
		std::function<bool()> _onPause = nullptr;

		std::chrono::steady_clock::time_point _time;
		std::atomic_uint16_t _frameId = { 0u };

        double _stealedTime = 0.0f;
		double _targetFrameTime = std::numeric_limits<float>::max();
		FpsLimitType _fpsLimitType = FpsLimitType::F_DONT_CARE;

        std::mutex _tasksMutex;
        WorkerTasks _tasks;
	};

}
