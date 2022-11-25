#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <functional>

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
					std::unique_lock<std::mutex> lock(_mutex);
					_condition.wait(lock, [this] { return isActive(); });
				}
			}
		}

		inline bool isActive() const { return !_paused.test(std::memory_order_acquire); }
		inline bool isAlive() const { return !_stop.test(std::memory_order_acquire); }

		inline void resume() {
			_paused.clear(std::memory_order_release);
			__notify();
		}

		inline void pause() {
			__pause();
			__wait();
		}

		inline void stop() {
			if (!_stop.test_and_set(std::memory_order_acq_rel)) {
				_paused.test_and_set(std::memory_order_release);
				if (_thread.joinable()) {
					_thread.join();
				}
			}
		}

	private:
		inline void __pause() { _paused.test_and_set(std::memory_order_release); }

		inline void __wait() {
			while (!_wait.test(std::memory_order_acquire)) {
				std::this_thread::yield();
			}
		}

		inline void __notify() { _condition.notify_one(); }

		std::thread _thread;
		std::atomic_flag _paused;
		std::atomic_flag _stop;
		std::atomic_flag _wait;

		std::mutex _mutex;
		std::condition_variable _condition;
		std::function<void()> _task;
	};

}
