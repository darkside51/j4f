#pragma once

#include "../Core/Common.h"
#include <utility>

namespace engine {

	struct Component {
		template <typename T>
		inline static constexpr uint16_t rtti() noexcept {
			return UniqueTypeId<Component>::getUniqueId<T>();
		}

		virtual ~Component() = default;
		virtual inline uint16_t rtti() const noexcept = 0;
	};

	template <typename T>
	struct ComponentImpl : public Component {

		ComponentImpl() = default;

		template <typename... Args>
		ComponentImpl(Args&&... args) : value(std::forward<Args>(args)...) {}

		inline uint16_t rtti() const noexcept override {
			return Component::rtti<T>();
		}

		T value;
	};

	template <typename T, typename... Args>
	Component* makeComponent(Args&&... args) {
		// create with memory pool?
		return new ComponentImpl<T>(std::forward<Args>(args)...);
	}

	struct AllocatorPlaceholder {
		template<typename T, typename... Args>
		auto make(Args&&... args) { return new T(std::forward<Args>(args)...); }
	};

	template <typename T, typename Allocator, typename... Args>
	auto allocateComponent(Allocator&& allocator, Args&&... args) -> ComponentImpl<T>* {
		if constexpr (std::is_pointer_v<Allocator> || is_smart_pointer_v<Allocator>) {
			return allocator->template make<ComponentImpl<T>>(std::forward<Args>(args)...);
		} else {
			return allocator.template make<ComponentImpl<T>>(std::forward<Args>(args)...);
		}
	}
}