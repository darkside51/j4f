#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>

namespace engine {
	template <class T>
	inline void hash_combine(std::size_t& s, T&& v) noexcept {
        // https://stackoverflow.com/questions/19195183/how-to-properly-hash-the-custom-struct
		s ^= std::hash<std::decay_t<T>>()(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
	}

	template <typename T>
	inline void hash_combo(size_t& hash, T&& v) noexcept {
		hash_combine(hash, std::forward<T>(v));
	}

	template <typename T, typename... Args>
	inline void hash_combo(size_t& hash, T&& v, Args&&... args) noexcept {
		hash_combine(hash, std::forward<T>(v));
		hash_combo(hash, std::forward<Args>(args)...);
	}

	template <typename... Args>
	inline size_t hash_combine(Args&&... args) noexcept {
		size_t hash = 0;
		hash_combo(hash, std::forward<Args>(args)...);
		return hash;
	}
    
	// composition hash example 
	/*inline size_t hash() const {
	    // use
		const size_t r = engine::hash_combine(a, b, s);
        // or
		size_t r = 0;
		engine::hash_combine(r, a);
		engine::hash_combine(r, b);
		engine::hash_combine(r, s);
	}*/

	template<typename T, typename = void>
	struct CustomHasher : std::false_type {};

	template<typename T>
	struct CustomHasher<T, std::enable_if_t<std::is_same<decltype(std::declval<T>().hash()), size_t>::value>> : std::true_type {};

	template <typename T>
	struct Hasher {
		inline std::size_t operator()(const T& k) const noexcept {
			if constexpr (CustomHasher<T>::value) {
				return k.hash();
			} else {
				return std::hash<T>{}(k);
			}
		}

		inline std::size_t operator()(T&& k) const noexcept {
			if constexpr (CustomHasher<T>::value) {
				return k.hash();
			} else {
				return std::hash<T>{}(k);
			}
		}
	};

//    template <typename T>
//    concept arithmetic = std::is_arithmetic_v<T>;
//
//    template <arithmetic T>
//    struct Hasher<T> {
//        inline std::size_t operator()(const T k) const noexcept { return k; }
//    };

    template<>
    struct Hasher<int8_t> {
        inline std::size_t operator()(const int8_t k) const noexcept { return k; }
    };

    template<>
    struct Hasher<int16_t> {
        inline std::size_t operator()(const int16_t k) const noexcept { return k; }
    };

    template<>
    struct Hasher<int32_t> {
        inline std::size_t operator()(const int32_t k) const noexcept { return k; }
    };

    template<>
    struct Hasher<int64_t> {
        inline std::size_t operator()(const int64_t k) const noexcept { return k; }
    };

    template<>
    struct Hasher<uint8_t> {
        inline std::size_t operator()(const uint8_t k) const noexcept { return k; }
    };

    template<>
    struct Hasher<uint16_t> {
        inline std::size_t operator()(const uint16_t k) const noexcept { return k; }
    };

    template<>
    struct Hasher<uint32_t> {
        inline std::size_t operator()(const uint32_t k) const noexcept { return k; }
    };

    template<>
    struct Hasher<uint64_t> {
        inline std::size_t operator()(const uint64_t k) const noexcept { return k; }
    };

    // for heterogenious search
    template<>
    struct Hasher<std::string> {
        using hash_type = std::hash<std::string_view>;
        using is_transparent = void;

        std::size_t operator()(const char* str) const { return hash_type{}(str); }
        std::size_t operator()(std::string_view str) const { return hash_type{}(str); }
        std::size_t operator()(std::string const& str) const { return hash_type{}(str); }
    };

    using String_hash = Hasher<std::string>;
}