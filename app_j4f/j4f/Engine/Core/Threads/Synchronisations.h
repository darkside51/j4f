#pragma once

#include <atomic>
#include <thread>

namespace engine {

	class AtomicLock {
	public:
		AtomicLock(std::atomic_bool& l) : _lock(l) {
			bool free = false;
			while (!_lock.compare_exchange_weak(free, true, std::memory_order_release, std::memory_order_relaxed)) {
				free = false;
				std::this_thread::yield();
			}
		}

		~AtomicLock() {
			_lock.store(false, std::memory_order_release);
		}

		static void wait(std::atomic_bool& l) {
			while (l.load(std::memory_order_acquire)) {
				std::this_thread::yield();
			}
		}

	private:
		std::atomic_bool& _lock;
	};

	class AtomicInc {
	public:
		AtomicInc(std::atomic_uint16_t& c) : _counter(c) {
			_counter.fetch_add(1, std::memory_order_release);
		}

		~AtomicInc() {
			_counter.fetch_sub(1, std::memory_order_release);
		}
	private:
		std::atomic_uint16_t& _counter;
	};

	class AtomicCounter {
	public:
		AtomicCounter() {
			_counter.store(0, std::memory_order_relaxed);
		}

		~AtomicCounter() {
			_counter.store(0, std::memory_order_release);
		}

		inline void waitForEmpty() const {
			while (_counter.load(std::memory_order_acquire) != 0) {
				std::this_thread::yield();
			}
		}

		inline uint16_t add() noexcept { return _counter.fetch_add(1, std::memory_order_release); }
		inline uint16_t sub() noexcept { return _counter.fetch_sub(1, std::memory_order_release); }

	private:
		std::atomic_int16_t _counter;
	};
}