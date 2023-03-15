#pragma once

#include "Linked_ptr.h"
#include "Linked_ext_ptr.h"

#ifdef _DEBUG
#include "../Utils/Debug/MemoryLeakChecker.h"
#endif

#include <cstdint>
#include <atomic>
#include <memory>

#define offset_of(type, p) (reinterpret_cast<size_t>(&((reinterpret_cast<type*>(0))->p)))

namespace engine {
	inline uint32_t alignValue(const uint32_t value, const uint32_t align) noexcept {
		return (value + align - 1) & ~(align - 1);
	}

	template<typename T>
	inline uint16_t getUniqueId() noexcept {
		static std::atomic_uint16_t staticId = 0;
		const uint16_t newId = staticId.fetch_add(1, std::memory_order_relaxed);
		return newId;
	}

	template<typename MainType>
	struct UniqueTypeId {
		template<typename T>
		static inline uint16_t getUniqueId() noexcept {
			static const uint16_t newId = staticId.fetch_add(1, std::memory_order_relaxed);
			return newId;
		}
	private:
		inline static std::atomic_uint16_t staticId = 0;
	};

	struct CommonType;
	using CommonUniqueTypeId = UniqueTypeId<CommonType>;

	template <typename>
	struct is_smart_pointer { enum : bool { value = false }; };

	template <typename T>
	struct is_smart_pointer<std::unique_ptr<T>> { enum : bool { value = true }; };

	template <typename T>
	struct is_smart_pointer<std::shared_ptr<T>> { enum : bool { value = true }; };

	template <typename T>
	struct is_smart_pointer<linked_ptr<T>> { enum : bool { value = true }; };

	template <typename T>
	struct is_smart_pointer<linked_ext_ptr<T>> { enum : bool { value = true }; };

	template<typename T>
	inline constexpr bool is_smart_pointer_v = engine::is_smart_pointer<T>::value;

	namespace static_inheritance {
		// класс для проверки возможности конвертирования T в U
		template <typename T, typename U>
		class Conversion {
			using first_type = uint8_t;
			class second_type { first_type dummy[2]; };
			static first_type ftest(U);
			static second_type ftest(...);
			static T makeT();
		public:
			 enum : bool {
				exists = sizeof(ftest(makeT())) == sizeof(first_type),
				same_type = false
			};
		};

		template <typename T>
		class Conversion<T, T> {
		public:
			 enum : bool {
				exists = true,
				same_type = true
			};
		};

		// проверка наследования типов на этапе компиляции
		template <typename T, typename U>
		constexpr bool classInheritClass() {
			return (Conversion<const T*, const U*>::exists) && (!Conversion<const U*, const void*>::same_type) && (!Conversion<const T, const U>::same_type);
		}
	}
}
