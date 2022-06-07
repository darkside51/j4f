#pragma once

#include <type_traits>

namespace engine {
	template <class T>
	inline void hash_combine(std::size_t& s, const T& v) { // https://stackoverflow.com/questions/19195183/how-to-properly-hash-the-custom-struct
		std::hash<T> h;
		s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
	}

	///////
	template <typename T>
	inline void hash_combo(size_t& hash, const T& v) {
		hash_combine(hash, v);
	}

	template <typename T, typename... Args>
	inline void hash_combo(size_t& hash, const T& v, Args&&... args) {
		hash_combine(hash, v);
		hash_combo(hash, std::forward<Args>(args)...);
	}

	template <typename... Args>
	inline size_t hash_combine(Args&&... args) {
		size_t hash = 0;
		hash_combo(hash, std::forward<Args>(args)...);
		return hash;
	}


	// composition hash example 
	/*inline size_t hash() const {
		const size_t r1 = engine::hash_combine(a, b, s);

		size_t r2 = 0;
		engine::hash_combine(result, a);
		engine::hash_combine(result, b);
		engine::hash_combine(result, s);
	}*/

	///////

	template<typename T, typename = void>
	struct CustomHasher : std::false_type {};

	template<typename T>
	struct CustomHasher<T, std::enable_if_t<std::is_same<decltype(std::declval<T>().hash()), size_t>::value>> : std::true_type {};

	template <typename T>
	struct Hasher {
		inline std::size_t operator()(const T& k) const {
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

	template<>
	struct Hasher<int8_t> {
		inline std::size_t operator()(const int8_t& k) const { return k; }
		inline std::size_t operator()(const int8_t&& k) const { return k; }
	};

	template<>
	struct Hasher<int16_t> {
		inline std::size_t operator()(const int16_t& k) const { return k; }
		inline std::size_t operator()(const int16_t&& k) const { return k; }
	};

	template<>
	struct Hasher<int32_t> {
		inline std::size_t operator()(const int32_t& k) const { return k; }
		inline std::size_t operator()(const int32_t&& k) const { return k; }
	};

	template<>
	struct Hasher<int64_t> {
		inline std::size_t operator()(const int64_t& k) const { return k; }
		inline std::size_t operator()(const int64_t&& k) const { return k; }
	};

	template<>
	struct Hasher<uint8_t> {
		inline std::size_t operator()(const uint8_t& k) const { return k; }
		inline std::size_t operator()(const uint8_t&& k) const { return k; }
	};

	template<>
	struct Hasher<uint16_t> {
		inline std::size_t operator()(const uint16_t& k) const { return k; }
		inline std::size_t operator()(const uint16_t&& k) const { return k; }
	};

	template<>
	struct Hasher<uint32_t> {
		inline std::size_t operator()(const uint32_t& k) const { return k; }
		inline std::size_t operator()(const uint32_t&& k) const { return k; }
	};

	template<>
	struct Hasher<uint64_t> {
		inline std::size_t operator()(const uint64_t& k) const { return k; }
		inline std::size_t operator()(const uint64_t&& k) const { return k; }
	};
}