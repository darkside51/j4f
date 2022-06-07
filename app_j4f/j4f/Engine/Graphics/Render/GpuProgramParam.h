#pragma once

#include <vector>
#include <cstdint>

namespace engine {

	struct GpuProgramParam {
		void* value = nullptr;
		bool mustFreeMemory = false;
		size_t size = 0;

		~GpuProgramParam() {
			if (mustFreeMemory) {
				free(value);
				mustFreeMemory = false;
			}

			size = 0;
			value = nullptr;
		}

		inline void setRawData(void* v, const size_t sz, const bool copyData) {
			if (copyData) {
				if (mustFreeMemory) {
					if (size != sz) {
						free(value);
						value = malloc(sz);
						memcpy(value, v, sz);
					} else {
						if (memcmp(value, v, sz) != 0) {
							memcpy(value, v, sz);
						}
					}
				} else {
					value = malloc(sz);
					memcpy(value, v, sz);
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

		template <typename T>
		inline void setValue(T* v, const uint32_t count, const bool copyData) {
			setRawData(v, sizeof(T) * count, copyData);
		}

		inline const void* getValue() const { return value; }
	};

	using GpuProgramParams = std::vector<engine::GpuProgramParam>;
}