#pragma once

#include "../Core/Common.h"

namespace engine {

	struct Component {
		template <typename T>
		inline static constexpr uint16_t rtti() {
			return UniqueTypeId<Component>::getUniqueId<T>();
		}

		virtual ~Component() = default;
		virtual inline uint16_t rtti() const = 0;
	};

	template <typename T>
	struct ComponentHolder : public Component {

		ComponentHolder() = default;

		template <typename... Args>
		ComponentHolder(Args&&... args) : value(std::forward<Args>(args)...) {}

		inline uint16_t rtti() const override {
			return Component::rtti<T>();
		}

		T value;
	};

	template <typename T, typename... Args>
	Component* makeComponent(Args&&... args) {
		return new ComponentHolder<T>(std::forward<Args>(args)...);
	}

}