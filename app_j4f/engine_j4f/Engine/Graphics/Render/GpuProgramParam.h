#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace engine {

	struct GpuProgramParam {
		void* value = nullptr;
		bool mustFreeMemory = false;
		size_t size = 0u;

		~GpuProgramParam() {
			if (mustFreeMemory) {
				free(value);
				mustFreeMemory = false;
			}

			size = 0u;
			value = nullptr;
		}

		inline void setRawData(void* v, const size_t sz, const bool copyData) {
			assert(sz != 0u);
			if (copyData) {
				if (mustFreeMemory) {
					if (size != sz) {
						free(value);
						if (value = malloc(sz)) {
							memcpy(value, v, sz);
						}
					} else {
						if (memcmp(value, v, sz) != 0) {
							memcpy(value, v, sz);
						}
					}
				} else {
					if (value = malloc(sz)) {
						memcpy(value, v, sz);
					}
				}

				mustFreeMemory = true;
			} else {
				if (mustFreeMemory) {
					free(value);
					mustFreeMemory = false;
				}

				value = v;
			}

			size = sz;
		}

		inline void setRawDataPart(void* v, const size_t sz, const size_t offset) {
			assert(sz != 0u);
			const size_t minSize = sz + offset;
			if (mustFreeMemory) {
				if (size <= minSize) {
					auto oldValue = value;
					if (value = malloc(minSize)) {
						memcpy(value, oldValue, size); // copy old data
						const auto ptr = reinterpret_cast<void*>(reinterpret_cast<size_t>(value) + offset);
						memcpy(ptr, v, sz); // copy new data
					}

					free(oldValue);
				} else {
					const auto ptr = reinterpret_cast<void*>(reinterpret_cast<size_t>(value) + offset);
					if (memcmp(ptr, v, sz) != 0) {
						memcpy(ptr, v, sz); // copy new data
					}
				}
			} else {
				if (value = malloc(minSize)) {
					const auto ptr = reinterpret_cast<void*>(reinterpret_cast<size_t>(value) + offset);
					memcpy(ptr, v, sz); // copy new data
				}
			}

			mustFreeMemory = true;
			size = std::max(sz, minSize);
		}

        inline void allocValueMemory(const size_t sz) {
            assert(sz != 0u);
            if (mustFreeMemory) {
                if (size != sz) {
                    realloc(value, sz);
                }
            } else {
                value = malloc(sz);
            }

            mustFreeMemory = true;
            size = sz;
        }

		template <typename T>
		inline void setValue(T* v, const uint32_t count, const bool copyData) noexcept {
			setRawData(v, sizeof(T) * count, copyData);
		}

		template <typename T>
		inline void setValueWithOffset(T* v, const uint32_t count, const size_t offset) noexcept {
			setRawDataPart(v, sizeof(T) * count, offset);
		}

		inline const void* getValue() const noexcept { return value; }
	};

	using GpuProgramParams = std::vector<engine::GpuProgramParam>;
}