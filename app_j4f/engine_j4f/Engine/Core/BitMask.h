#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace engine {

	template <typename T>
	class BitMaskImpl {
		using element_type = T;
		inline static constexpr uint8_t count_bits = sizeof(element_type) * 8;

	public:
		BitMaskImpl() = default; // �� ���� ��������� ����� � ������������, � ������ ����� ��������� � ������ ������
		~BitMaskImpl() = default;

		BitMaskImpl(const BitMaskImpl& mask) {
			const size_t sz = mask._mask.size();
			_mask.resize(sz);
			memcpy(&_mask[0], &mask._mask[0], sz * sizeof(element_type));
		}

		BitMaskImpl(BitMaskImpl&& mask) noexcept {
			const size_t sz = mask._mask.size();
			_mask.resize(sz);
			_mask = std::move(mask._mask);
		}

		template <typename INTEGER_TYPE>
		inline bool checkBit(const INTEGER_TYPE bit) const noexcept {
			static_assert(std::is_integral_v<INTEGER_TYPE>, "type not integer");

			const auto innerBit = static_cast<uint32_t>(bit);
			const auto bitsCount = static_cast<uint32_t>(_mask.size()) * count_bits;
			if (innerBit < bitsCount) {
				const uint32_t num = innerBit / count_bits;
				return _mask[num] & (1 << (innerBit - (count_bits * num)));
			}

			return false;
		}

		template <typename INTEGER_TYPE>
		inline void setBit(const INTEGER_TYPE bit, const uint8_t value) {
			static_assert(std::is_integral_v<INTEGER_TYPE>, "type not integer");

			const auto sz = static_cast<uint32_t>(_mask.size());
			const auto innerBit = static_cast<uint32_t>(bit);
			const uint32_t num = innerBit / count_bits;

			if (sz <= num) { _mask.resize(num + 1, 0); }

			switch (value) {
				case 0:
					_mask[num] &= ~(1 << (innerBit - (count_bits * num)));
					break;
				default:
					_mask[num] |= (1 << (innerBit - (count_bits * num)));
					break;
			}
		}

		inline bool checkMaskBits(const BitMaskImpl& outerBM) const { // ���������, ��� � ����� outerBM �������� ��� ������� �� �� ����, ��� � � _mask
			const size_t selfSize = _mask.size();
			const size_t outerSize = outerBM._mask.size();

			const size_t minSize = std::min(selfSize, outerSize);

			for (size_t i = 0; i < minSize; ++i) {
				if ((_mask[i] & outerBM._mask[i]) != _mask[i]) { // � outerBM ����� ���� �������� � ������ ����, �� ��, ��� _mask ����� ������ ���� ��������
					return false;
				}
			}

			if (selfSize > outerSize) {
				// ������, ����� � ������� ����� ��������������� ������ ���, �� �������� �������, ��� ��� ����, �� �������� � outerBM �������
				for (size_t i = outerSize; i < selfSize; ++i) {
					if (_mask[i]) return false;
				}
			}

			return true;
		}

		inline bool operator== (const BitMaskImpl& bm) const {
			const size_t sz = _mask.size();
			const size_t sz2 = bm._mask.size();

			if (sz != sz2) return false;

			return (memcmp(&_mask[0], &bm._mask[0], sz * sizeof(element_type)) == 0);
		}

		inline BitMaskImpl& operator= (const BitMaskImpl& bm) {
			const size_t sz = bm._mask.size();
			_mask.resize(sz);
			memcpy(&_mask[0], &bm._mask[0], sz * sizeof(element_type));
			return *this;
		}

		inline BitMaskImpl& operator= (BitMaskImpl&& bm) noexcept {
			const size_t sz = bm._mask.size();
			_mask.resize(sz);
			_mask = std::move(bm._mask);
			return *this;
		}

		inline void reset() { _mask.clear(); }
		inline void nullify() { memset(&_mask[0], 0, _mask.size() * sizeof(element_type)); }

	private:
		std::vector<element_type> _mask;
	};

	using BitMask32 = BitMaskImpl<uint32_t>;
	using BitMask64 = BitMaskImpl<uint64_t>;

	////////////// some helpers

	template <typename M, typename... Args>
    inline typename std::enable_if<sizeof...(Args) == 0>::type fillBitMaskType(M&) { }

	template <typename M, typename T, typename... Args>
    inline void fillBitMaskType(M& mask) {
		mask.setBit(T::template rtti<T>(), 1);
		fillBitMaskType<Args...>(mask);
	}

	template<typename M, typename... Args>
	inline void changeBitMaskTypesTo(M& mask) {
		mask.nullify();
		fillBitMaskType<Args...>(mask);
	}

	template<typename M, typename... Args>
	inline M createBitMaskWithTypes() { // const BitMask32 mask = createBitMaskWithTypes<Component1, Component2, Component3, Component4>();
		M mask;
		fillBitMaskType<Args...>(mask);
		return mask;
	}
}