#pragma once

#include "Common.h"
#include <atomic> 

namespace engine {

	// don't know why i crate this

	template<typename T, typename Deleter = void>
	class handler_ptr;

	template <typename T, typename Deleter = void>
	class ValueStorage {
		friend class handler_ptr<T, Deleter>;
	public:
		ValueStorage(T* v) : _value(v), _counter(1) {}
		ValueStorage(T* v, void* m) : _value(v), _counter(1), _memory(m) {}
		~ValueStorage() {
			if (_value) {
				if constexpr (static_inheritance::Conversion<Deleter, void>::same_type) {
					if (_memory) {
						_value->~T();
						free(_memory);
						_memory = nullptr;
					} else {
						delete _value;
						_value = nullptr;
					}
				} else {
					Deleter(_value, _memory);
					_value = nullptr;
				}
			}
		};

		inline void inc() { _counter.fetch_add(1, std::memory_order_acq_rel) + 1; }
		inline void dec() { 
			if (_counter.fetch_sub(1, std::memory_order_acq_rel) == 1) { 
				if (_memory) {
					this->~ValueStorage();
				} else {
					delete this;
				}
			}
		}

		inline int32_t use_count() const { return _counter.load(std::memory_order_acquire); }

	private:
		T* _value;
		std::atomic_int32_t _counter;

		void* _memory = nullptr;
	};

	template<typename T, typename Deleter>
	class handler_ptr {
		using Storage = ValueStorage<T, Deleter>;
	public:
		handler_ptr() : _ptr(new Storage(nullptr)) {}
		handler_ptr(T* v) : _ptr(new Storage(v)) {}
		handler_ptr(Storage* s) : _ptr(s) {}
		handler_ptr(const handler_ptr& ptr) {
			ptr._ptr.load(std::memory_order_acquire)->inc();
			_ptr.store(ptr._ptr, std::memory_order_release);
		}
		handler_ptr(handler_ptr&& ptr) noexcept {
			_ptr.store(ptr._ptr, std::memory_order_release);
			ptr._ptr.store(nullptr, std::memory_order_release);
		}

		~handler_ptr() {
			if (auto&& s = _ptr.exchange(nullptr, std::memory_order_acq_rel)) s->dec();
		}

		handler_ptr& operator= (T* v) {
			_ptr.exchange(new Storage(v), std::memory_order_acq_rel)->dec();
			return *this;
		}
		handler_ptr& operator= (const handler_ptr& ptr) {
			if (this == &ptr) return *this;
			ptr._ptr.load(std::memory_order_acquire)->inc();
			_ptr.exchange(ptr._ptr, std::memory_order_acq_rel)->dec();
			return *this;
		}
		handler_ptr& operator= (handler_ptr&& ptr) noexcept {
			if (this == &ptr) return *this;
			_ptr.exchange(ptr._ptr, std::memory_order_acq_rel)->dec();
			ptr._ptr.store(nullptr, std::memory_order_release);
			return *this;
		}

		inline bool operator== (const handler_ptr& ptr) const {
			return _ptr.load(std::memory_order_acquire)->_value == ptr._ptr.load(std::memory_order_acquire)->_value;
		}

		inline bool operator!= (const handler_ptr& ptr) const {
			return _ptr.load(std::memory_order_acquire)->_value != ptr._ptr.load(std::memory_order_acquire)->_value;
		}

		inline T* operator-> () const { return _ptr.load(std::memory_order_acquire)->_value; }
		inline T* get() const { return _ptr.load(std::memory_order_acquire)->_value; }

		inline int32_t use_count() const { return _ptr.load(std::memory_order_acquire)->use_count(); }
		inline bool unique() const { return (use_count() == 0); }

	private:
		std::atomic<Storage*> _ptr;
	};

	template<typename T, typename D, typename...Args>
	inline handler_ptr<T, D> make_handler(Args&&... args) {

		constexpr size_t size = sizeof(T) + sizeof(ValueStorage<T, D>);
		void* memory = malloc(size);
		T* value = new(memory) T(std::forward<Args>(args)...);
		ValueStorage<T, D>* storage = new(reinterpret_cast<void*>(reinterpret_cast<size_t>(memory) + sizeof(T))) ValueStorage<T, D>(value, memory);

		return handler_ptr<T, D>(storage);

		//return handler_ptr<T, D>(new T(std::forward<Args>(args)...));
	}

}