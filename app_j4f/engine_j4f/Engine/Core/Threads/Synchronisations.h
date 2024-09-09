#pragma once

#include <atomic>
#include <thread>

namespace engine {

	class SpinLock {
	public:
		inline void lock() noexcept {
			while (_flag.test_and_set(std::memory_order_acquire)) {
				std::this_thread::yield();
			}
		}

		inline void unlock() noexcept {
			_flag.clear(std::memory_order_release);
		}

	private:
		std::atomic_flag _flag{};
	};

	class AtomicLock {
	public:
		explicit AtomicLock(std::atomic_bool& l) : _lock(l) {
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
			while (l.load(std::memory_order_consume)) {
				std::this_thread::yield();
			}
		}

	private:
		std::atomic_bool& _lock;
	};

	class AtomicLockF {
	public:
		explicit AtomicLockF(std::atomic_flag& l) : _lock(l) {
			while (_lock.test_and_set(std::memory_order_release)) {
				std::this_thread::yield();
			}
		}

		~AtomicLockF() {
			_lock.clear(std::memory_order_release);
		}

		static void wait(std::atomic_flag& l) {
			while (l.test(std::memory_order_release)) {
				std::this_thread::yield();
			}
		}

	private:
		std::atomic_flag& _lock;
	};

	class AtomicInc {
	public:
		explicit AtomicInc(std::atomic_uint16_t& c) : _counter(c) {
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
			while (_counter.load(std::memory_order_consume) != 0) {
				std::this_thread::yield();
			}
		}

		inline uint16_t add() noexcept { return _counter.fetch_add(1, std::memory_order_release); }
		inline uint16_t sub() noexcept { return _counter.fetch_sub(1, std::memory_order_release); }

	private:
		std::atomic_int16_t _counter;
	};
}