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

	template <typename T>
	class linked_ext_ptr;

	template<typename T>
	class linked_weak_ptr_control;

	template <typename T>
	class linked_weak_ptr;

	template <typename T>
	class control_block {
		friend class linked_weak_ptr<T>;
		friend class linked_ptr<T>;
		friend class linked_ext_ptr<T>;
		friend class linked_weak_ptr_control<T>;

		using type = control_block<T>;
		using counter_type = uint32_t;

		inline uint32_t _decrease_counter()			{ return --__counter; }
		inline uint32_t _increase_counter()			{ return ++__counter; }
		inline uint32_t _use_count() const			{ return __counter; }

		uint32_t __counter = 0;
	};

	template <typename T>
	class atomic_control_block {
		friend class linked_weak_ptr<T>;
		friend class linked_ptr<T>;
		friend class linked_ext_ptr<T>;
		friend class linked_weak_ptr_control<T>;

		using type = atomic_control_block<T>;
		using counter_type = std::atomic_uint32_t;

		inline uint32_t _decrease_counter()			{ return __counter.fetch_sub(1, std::memory_order_release) - 1; }
		inline uint32_t _increase_counter()			{ return __counter.fetch_add(1, std::memory_order_release) + 1; }
		inline uint32_t _use_count() const			{ return __counter.load(std::memory_order_consume); }

		std::atomic_uint32_t __counter = 0;
	};

	template <typename T> // requires destroy_requirement<T>
	class linked_ptr {
	public:
		using element_type = T;
		
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

		inline linked_ptr& operator= (nullptr_t) {
			if (_ptr) {
				_decrease_counter();
				_ptr = nullptr;
			}
			return *this;
		}

		inline linked_ptr& operator= (const linked_ptr& p) {
			if (p._ptr != _ptr) {
				_decrease_counter();
				_ptr = p._ptr;
				_increase_counter();
			}
			return *this;
		}

		inline linked_ptr& operator= (linked_ptr&& p) noexcept {
			if (p._ptr != _ptr) {
				_decrease_counter();
			}

			_ptr = p._ptr;
			p._ptr = nullptr;
			return *this;
		}

		inline const linked_ptr& operator= (element_type* p) {
			if (p != _ptr) {
				_decrease_counter();
				_ptr = p;
				_increase_counter();
			}
			return *this;
		}

		inline const linked_ptr& operator= (const element_type* p) {
			if (p != _ptr) {
				_decrease_counter();
				_ptr = const_cast<element_type*>(p);
				_increase_counter();
			}
			return *this;
		}

		inline bool operator== (const linked_ptr& p) const { return _ptr == p._ptr; }
		inline bool operator== (const element_type* p) const { return _ptr == p; }

		inline element_type* operator->() { return _ptr; }
		inline const element_type* operator->() const { return _ptr; }

		inline operator bool() const noexcept { return _ptr != nullptr; }

		inline uint32_t use_count() const { return _ptr->_use_count(); }

		inline element_type* get() { return _ptr; }
		inline const element_type* get() const { return _ptr; }

	protected:
		inline void _increase_counter() noexcept {
			if (_ptr) {
				_ptr->_increase_counter();
			}
		}

		inline void _decrease_counter() noexcept {
			if (_ptr && _ptr->_decrease_counter() == 0) {
				delete _ptr;
			}
		}

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
		auto p = static_cast<typename engine::linked_ptr<T>::element_type*>(const_cast<engine::linked_ptr<U>&>(r).get());
		return engine::linked_ptr<T>(p);
	}
	
	template< class T, class U >
	engine::linked_ptr<T> dynamic_pointer_cast(const engine::linked_ptr<U>& r) noexcept {
		if (auto p = dynamic_cast<typename engine::linked_ptr<T>::element_type*>(const_cast<engine::linked_ptr<U>&>(r).get())) {
			return engine::linked_ptr<T>(p);
		} else {
			return engine::linked_ptr<T>();
		}
	}
	
	template< class T, class U >
	engine::linked_ptr<T> const_pointer_cast(const engine::linked_ptr<U>& r) noexcept {
		auto p = const_cast<typename engine::linked_ptr<T>::element_type*>(const_cast<engine::linked_ptr<U>&>(r).get());
		return engine::linked_ptr<T>(p);
	}
	
	template< class T, class U >
	engine::linked_ptr<T> reinterpret_pointer_cast(const engine::linked_ptr<U>& r) noexcept {
		auto p = reinterpret_cast<typename engine::linked_ptr<T>::element_type*>(const_cast<engine::linked_ptr<U>&>(r).get());
		return engine::linked_ptr<T>(p);
	}
}