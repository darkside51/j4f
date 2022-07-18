#pragma once

#include <type_traits>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <memory>

namespace engine {

	template <typename T, int C>
	class Hierarchy;

	template <typename T, int C>
	struct HierarchyChildrenType;

	template <typename T>
	struct HierarchyChildrenType<T, 0> { using type = Hierarchy<T, 0>*; };

	template <typename T>
	struct HierarchyChildrenType<T, 1> { using type = std::shared_ptr<Hierarchy<T, 1>>; };

	template <typename T, int C>
	class Hierarchy {
		using children_type = typename HierarchyChildrenType<T, C>::type;
	public:
		template <typename... Args>
		Hierarchy(Args&&... args) : _value(std::forward<Args>(args)...) {}

		Hierarchy(T&& v) : _value(std::move(v)) {}
		Hierarchy(const T& v) : _value(v) {}

		~Hierarchy() {
			if constexpr (std::is_pointer<children_type>()) {
				for (children_type& ch : _children) {
					delete ch;
				}
			}
		
			_children.clear();

			invalidateLinks();
		}

		Hierarchy(Hierarchy&& h) noexcept : _value(std::move(h._value)), _children(std::move(h._children)), _parent(h._parent), _next(h._next) {
			h._parent = nullptr;
			h._next = nullptr;
			h._children.clear();
		}

		const Hierarchy& operator= (Hierarchy&& h) noexcept {
			_value = std::move(h._value);
			_children = std::move(h._children);
			_parent = h._parent;
			_next = h._next;

			h._parent = nullptr;
			h._next = nullptr;
			h._children.clear();

			return *this;
		}

		Hierarchy(const Hierarchy& h) = delete;
		const Hierarchy& operator= (const Hierarchy& h) = delete;

		inline void invalidateLinks() {
			if (_prev) {
				_prev->_next = _next;
			}

			if (_next) {
				_next->_prev = _prev;
			}

			_parent = nullptr;
			_next = nullptr;
			_prev = nullptr;
		}

		inline void reserve(const size_t sz) {
			_children.reserve(sz);
		}

		void addChild(children_type c) {
			if (!_children.empty()) { 
				if constexpr (std::is_pointer<children_type>()) {
					_children.back()->_next = c;
					c->_prev = _children.back();
				} else {
					_children.back()->_next = c.get();
					c->_prev = _children.back().get();
				}
			}
			c->_parent = this;
			_children.push_back(c);
		}

		void removeChild(children_type c) {
			auto it = std::remove(_children.begin(), _children.end(), c);
			if (it != _children.end()) {
				if constexpr (std::is_pointer<children_type>()) {
					delete c;
				} else {
					c->invalidateLinks();
				}
				_children.erase(it, _children.end());
			}
		}

		const std::vector<children_type>& children() const { return _children; }

		template<typename F, typename ...Args>
		inline void execute(F&& f, Args&&... args) { // no recursive execute f for all hierarchy
			const Hierarchy* current = this;
			while (current) {
				if (f(const_cast<Hierarchy*>(current), std::forward<Args>(args)...)) { // can skip some
					const std::vector<children_type>& children = current->_children;
					if (!children.empty()) {
						if constexpr (std::is_pointer<children_type>()) {
							current = current->_children[0];
						} else {
							current = current->_children[0].get();
						}
						continue;
					} else if (current != this) {
						if (current->_next) {
							current = current->_next;
							continue;
						} else if (current->_parent && current->_parent != this) {
							while (current->_parent) {
								current = current->_parent;
								if (current == this) return;
								if (current->_next) {
									current = current->_next;
									break;
								}
							}
							continue;
						} else {
							break;
						}
					} else {
						break;
					}
				} else if (current != this) {
					if (current->_next) {
						current = current->_next;
						continue;
					} else if (current->_parent && current->_parent != this) {
						while (current->_parent) {
							current = current->_parent;
							if (current == this) return;
							if (current->_next) {
								current = current->_next;
								break;
							}
						}
						continue;
					} else {
						break;
					}
				} else {
					break;
				}
			}
		}

		template<typename F, typename Fcheck, typename ...Args>
		inline void r_execute(F&& f, Fcheck&& f2, Args&&... args) { // no recursive reverse execute f for all hierarchy
			const Hierarchy* current = this;
			while (current) {
				if (f2(const_cast<Hierarchy*>(current)) || current == this) { // can skip some
					const std::vector<children_type>& children = current->_children;
					if (!children.empty()) {
						if constexpr (std::is_pointer<children_type>()) {
							current = current->_children[0];
						} else {
							current = current->_children[0].get();
						}
						continue;
					}
					else if (current != this) {
						f(const_cast<Hierarchy*>(current), std::forward<Args>(args)...); // call for childless element
						if (current->_next) {
							current = current->_next;
							continue;
						} else if (current->_parent) {
							while (current->_parent) {
								current = current->_parent;
								f(const_cast<Hierarchy*>(current), std::forward<Args>(args)...); // call for parent
								if (current == this) return;
								if (current->_next) {
									current = current->_next;
									break;
								}
							}
							continue;
						} else {
							break;
						}
					} else {
						break;
					}
				} else if (current != this) {
					if (current->_next) {
						current = current->_next;
						continue;
					} else if (current->_parent) {
						while (current->_parent) {
							current = current->_parent;
							f(const_cast<Hierarchy*>(current), std::forward<Args>(args)...); // call for parent
							if (current == this) return;
							if (current->_next) {
								current = current->_next;
								break;
							}
						}
						continue;
					}
					else {
						break;
					}
				}
			}
		}

		template<typename E, typename ...Args>
		inline void execute_with(Args&&... args) { // no recursive execute f for all hierarchy
			const Hierarchy* current = this;
			while (current) {
				if (E::_(const_cast<Hierarchy*>(current), std::forward<Args>(args)...)) { // can skip some
					const std::vector<children_type>& children = current->_children;
					if (!children.empty()) {
						if constexpr (std::is_pointer<children_type>()) {
							current = current->_children[0];
						} else {
							current = current->_children[0].get();
						}
						continue;
					} else if (current != this) {
						if (current->_next) {
							current = current->_next;
							continue;
						} else if (current->_parent && current->_parent != this) {
							while (current->_parent) {
								current = current->_parent;
								if (current == this) return;
								if (current->_next) {
									current = current->_next;
									break;
								}
							}
							continue;
						} else {
							break;
						}
					} else {
						break;
					}
				} else if (current != this) {
					if (current->_next) {
						current = current->_next;
						continue;
					} else if (current->_parent && current->_parent != this) {
						while (current->_parent) {
							current = current->_parent;
							if (current == this) return;
							if (current->_next) {
								current = current->_next;
								break;
							}
						}
						continue;
					} else {
						break;
					}
				} else {
					break;
				}
			}
		}

		template<typename E, typename Echeck, typename ...Args>
		inline void r_execute_with(Args&&... args) { // no recursive reverse execute f for all hierarchy
			const Hierarchy* current = this;
			while (current) {
				if (Echeck::_(const_cast<Hierarchy*>(current)) || current == this) { // can skip some
					const std::vector<children_type>& children = current->_children;
					if (!children.empty()) {
						if constexpr (std::is_pointer<children_type>()) {
							current = current->_children[0];
						} else {
							current = current->_children[0].get();
						}
						continue;
					} else if (current != this) {
						E::_(const_cast<Hierarchy*>(current), std::forward<Args>(args)...); // call for childless element
						if (current->_next) {
							current = current->_next;
							continue;
						} else if (current->_parent) {
							while (current->_parent) {
								current = current->_parent;
								E::_(const_cast<Hierarchy*>(current), std::forward<Args>(args)...); // call for parent
								if (current == this) return;
								if (current->_next) {
									current = current->_next;
									break;
								}
							}
							continue;
						} else {
							break;
						}
					} else {
						break;
					}
				} else if (current != this) {
					if (current->_next) {
						current = current->_next;
						continue;
					} else if (current->_parent) {
						while (current->_parent) {
							current = current->_parent;
							E::_(const_cast<Hierarchy*>(current), std::forward<Args>(args)...); // call for parent
							if (current == this) return;
							if (current->_next) {
								current = current->_next;
								break;
							}
						}
						continue;
					} else {
						break;
					}
				}
			}
		}

		inline T& value() { return _value; }
		inline const T& value() const { return _value; }

		inline const Hierarchy* getParent() const { return _parent; }
		inline const Hierarchy* getPrev() const { return _prev; }
		inline const Hierarchy* getNext() const { return _next; }

	private:
		T _value;
		const Hierarchy* _parent = nullptr;
		Hierarchy* _next = nullptr;
		Hierarchy* _prev = nullptr;
		std::vector<children_type> _children;
	};

	template <typename T>
	using HierarchyRaw = Hierarchy<T, 0>;

	template <typename T>
	using HierarchyShared = Hierarchy<T, 1>;
}