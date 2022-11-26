#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <functional>
#include "Synchronisations.h"

namespace engine {

	class WorkerThread {
	public:
		template <typename T, typename F, typename... Args>
		explicit WorkerThread(F T::* f, T* t, Args&&... args) : 
			_task([f, t, args...]() { (t->*f)(std::forward<Args>(args)...); }), 
			_thread(&WorkerThread::work, this) {
		}

		template <typename F, typename... Args>
		explicit WorkerThread(F&& f, Args&&... args) :
			_task([f, args...]() { f(std::forward<Args>(args)...); }),
			_thread(&WorkerThread::work, this) {
		}

		~WorkerThread() {
			stop();
		}

		WorkerThread(const WorkerThread&) = delete;
		const WorkerThread& operator= (WorkerThread&) = delete;

		inline void work() {
			while (isAlive()) {
				_wait.clear(std::memory_order_release);

				while (isActive()) {
					_task();
				}

				_wait.test_and_set(std::memory_order_acq_rel);

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
							requestResume(); // remove pause flag if _onPause returnedz false
						}
					} else {
						sleep();
					}
				}
			}
		}

		inline bool isActive() const { return !_paused.test(std::memory_order_acquire); }
		inline bool isAlive() const { return !_stop.test(std::memory_order_acquire); }

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

		std::function<void()> _task = nullptr;

		std::atomic_flag _callbackLock;
		std::function<bool()> _onPause = nullptr;
	};

}
