#pragma once

#include <cstdint>
#include <atomic>

namespace engine {

	template<typename T>
	concept has_counter_functions = requires(T v) {
		v._decrease_counter();
		v._increase_counter();
		v._use_count();
	};

	template<typename T>
	concept destroy_requirement = requires(T v) {
		v.destroy();
	};

	template <typename T> // requires has_counter_functions<T>
	class linked_ptr;

	template<typename T>
	class linked_weak_ptr_control;

	template <typename T>
	class linked_weak_ptr;

	template <typename T>
	class control_block {
		using type = control_block<T>;
		using counter_type = uint32_t;

		friend class linked_weak_ptr<T>;
		friend class linked_ptr<T>;
		friend class linked_weak_ptr_control<T>;

		inline uint32_t _decrease_counter()			{ return --__counter; }
		inline uint32_t _increase_counter()			{ return ++__counter; }
		inline uint32_t _use_count() const			{ return __counter; }

		uint32_t __counter = 0;
	};

	template <typename T>
	class atomic_control_block {
		using type = atomic_control_block<T>;
		using counter_type = std::atomic_uint32_t;

		friend class linked_weak_ptr<T>;
		friend class linked_ptr<T>;
		friend class linked_weak_ptr_control<T>;

		inline uint32_t _decrease_counter()			{ return __counter.fetch_sub(1, std::memory_order_consume) - 1; }
		inline uint32_t _increase_counter()			{ return __counter.fetch_add(1, std::memory_order_consume) + 1; }
		inline uint32_t _use_count() const			{ return __counter.load(std::memory_order_acquire); }

		std::atomic_uint32_t __counter = 0;
	};

	template<typename T>
	class linked_weak_ptr_control {
		friend class linked_weak_ptr<T>;

		T::type _counter;
		T* _target = nullptr;
	};

	// https://stackoverflow.com/questions/2400458/is-there-a-boostweak-intrusive-pointer
	template <typename T>
	class linked_weak_ptr {
	public:
		linked_weak_ptr() = default;

		inline linked_ptr<T> lock() {
			return nullptr;
		}

		inline bool expired() const {
			return false;
		}

	private:
		linked_weak_ptr_control<T>* _control = nullptr;
	};

	template <typename T> // requires destroy_requirement<T>
	class linked_ptr {
	public:
		using element_type = T;
		using weak_control_type = linked_weak_ptr_control<T>;
		using weak = linked_weak_ptr<T>;

		linked_ptr() = default;

		~linked_ptr() {
			_decrease_counter();
			_ptr = nullptr;
		}

		linked_ptr(element_type* ptr) : _ptr(ptr) {
			_increase_counter();
		}

		linked_ptr(const linked_ptr& p) : _ptr(p._ptr) {
			_increase_counter();
		}

		linked_ptr(linked_ptr&& p) noexcept : _ptr(p._ptr) {
			p._ptr = nullptr;
		}

		inline const linked_ptr& operator= (const linked_ptr& p) {
			_decrease_counter();
			_ptr = p._ptr;
			_increase_counter();
			return *this;
		}

		inline const linked_ptr& operator= (linked_ptr&& p) noexcept {
			_decrease_counter();
			_ptr = p._ptr;
			p._ptr = nullptr;
			return *this;
		}

		inline const linked_ptr& operator= (element_type* p) {
			_decrease_counter();
			_ptr = p;
			_increase_counter();
			return *this;
		}

		inline const linked_ptr& operator= (const element_type* p) {
			_decrease_counter();
			_ptr = const_cast<element_type*>(p);
			_increase_counter();
			return *this;
		}

		inline bool operator== (const linked_ptr& p) { return _ptr == p._ptr; }
		inline bool operator== (const element_type* p) { return _ptr == p; }

		inline element_type* operator->() { return _ptr; }
		inline const element_type* operator->() const { return _ptr; }

		inline operator bool() const noexcept { return _ptr != nullptr; }

		inline uint32_t use_count() const { return _ptr->_use_count(); }

		inline element_type* get() { return _ptr; }
		inline const element_type* get() const { return _ptr; }

	private:
		inline void _increase_counter() {
			if (_ptr) {
				_ptr->_increase_counter();
			}
		}

		inline void _decrease_counter() {
			if (_ptr && _ptr->_decrease_counter() == 0) {
				delete _ptr;
			}
		}

		weak_control_type* _weak_control = nullptr;
		element_type* _ptr = nullptr;
	};

	template <typename T, typename... Args>
	inline linked_ptr<T> make_linked(Args&&... args) {
		return linked_ptr<T>(new T(std::forward<Args>(args)...));
	}
}

namespace std {
	template< class T, class U >
	engine::linked_ptr<T> static_pointer_cast(const engine::linked_ptr<U>& r) noexcept {
		auto p = static_cast<typename engine::linked_ptr<T>::element_type*>(r.get());
		return engine::linked_ptr<T>(p);
	}
	
	template< class T, class U >
	engine::linked_ptr<T> dynamic_pointer_cast(const engine::linked_ptr<U>& r) noexcept {
		if (auto p = dynamic_cast<typename engine::linked_ptr<T>::element_type*>(r.get())) {
			return engine::linked_ptr<T>(p);
		} else {
			return engine::linked_ptr<T>();
		}
	}
	
	template< class T, class U >
	engine::linked_ptr<T> const_pointer_cast(const engine::linked_ptr<U>& r) noexcept {
		auto p = const_cast<typename engine::linked_ptr<T>::element_type*>(r.get());
		return engine::linked_ptr<T>(p);
	}
	
	template< class T, class U >
	engine::linked_ptr<T> reinterpret_pointer_cast(const engine::linked_ptr<U>& r) noexcept {
		auto p = reinterpret_cast<typename engine::linked_ptr<T>::element_type*>(r.get());
		return engine::linked_ptr<T>(p);
	}
}