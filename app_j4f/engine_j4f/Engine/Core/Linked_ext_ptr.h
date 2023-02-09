#pragma once

#include "Linked_ptr.h"

namespace engine {

	template<typename T>
	class linked_weak_ptr_control {
		friend class linked_weak_ptr<T>;
		friend class linked_ext_ptr<T>;

		T::type _counter;
		T* _target = nullptr;
	};

	// https://stackoverflow.com/questions/2400458/is-there-a-boostweak-intrusive-pointer
	template <typename T>
	class linked_weak_ptr {
		using weak_control_type = linked_weak_ptr_control<T>;
	public:
		linked_weak_ptr() = default;

		linked_weak_ptr(const linked_ext_ptr<T>& ptr) : _control(ptr->_weak_control) {}

		linked_weak_ptr(const linked_weak_ptr& w) : _control(w._control) {
			_increase_counter();
		}

		linked_weak_ptr(linked_weak_ptr&& w) noexcept : _control(w._control) {
			w._control = nullptr;
		}

		~linked_weak_ptr() {
			_decrease_counter();
			_control = nullptr;
		}

		inline const linked_weak_ptr& operator= (const linked_weak_ptr& p) {
			_decrease_counter();
			_control = p._control;
			_increase_counter();
			return *this;
		}

		inline const linked_weak_ptr& operator= (linked_weak_ptr&& p) noexcept {
			_decrease_counter();
			_control = p._control;
			p._control = nullptr;
			return *this;
		}

		inline linked_ext_ptr<T> lock() {
			if (_control && _control->_counter._use_count()) {
				return linked_ext_ptr<T>(_control);
			}
			return nullptr;
		}

		inline bool expired() const {
			if (_control && _control->_counter._use_count()) { return false; }
			return true;
		}

	private:
		inline void _increase_counter() {
			if (_control) {
				_control->_counter._increase_counter();
			}
		}

		inline void _decrease_counter() {
			if (_control && _control->_counter._decrease_counter() == 0) {
				_control->_target = nullptr;
			}
		}

		weak_control_type* _control = nullptr;
	};

	template <typename T>
	class linked_ext_ptr : public linked_ptr<T> {
		friend class linked_weak_ptr<T>;
	public:
		using base = linked_ptr<T>;
		using weak_control_type = linked_weak_ptr_control<T>;
		using weak = linked_weak_ptr<T>;

		linked_ext_ptr(weak_control_type* control) : linked_ptr<T>(control->_target), _weak_control(control) { }

		template <typename... Args>
		linked_ext_ptr(Args&&... args) : linked_ptr<T>(std::forward<Args>(args)...), _weak_control(new weak_control_type()) { }

/*
		inline const linked_ext_ptr& operator= (const linked_ext_ptr& p) {
			_decrease_counter();
			_control = p._control;
			_increase_counter();
			return *this;
		}

		inline const linked_ext_ptr& operator= (linked_ext_ptr&& p) noexcept {
			_decrease_counter();
			_control = p._control;
			p._ptr = nullptr;
			return *this;
		}
		*/
	private:
		inline void _increase_counter() {
			base::_increase_counter();
		}

		inline void _decrease_counter() {
			if (base::_ptr && base::_ptr->_decrease_counter() == 0) {
				if (_weak_control && _weak_control->_counter._use_count() == 0) {
					delete base::_ptr;
					delete _weak_control;
				}
			}
		}

		weak_control_type* _weak_control = nullptr;
	};

}