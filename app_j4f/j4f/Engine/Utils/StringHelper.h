#pragma once

#include <string>
#include <cstdint>
#include <cstdio>

namespace engine {

	template <typename...Args>
	inline const char* fmt_string(const char* fmt, Args&&...args) {
		constexpr uint16_t max_buffer_size = 1024;
		static thread_local char buffer[max_buffer_size]; // max_buffer_size bytes memory for every thread, with static allocation
		snprintf(buffer, max_buffer_size, fmt, std::forward<Args>(args)...);
		return buffer;
	}

	template <typename...Args>
	inline std::string fmtString(const char* fmt, Args&&...args) {
		return std::string(fmt_string(fmt, std::forward<Args>(args)...));
	}

	inline uint16_t stringReplace(std::string& source, const std::string& find, const std::string& replace) {
		if (find.empty()) { return 0; }

		uint16_t num = 0;
		const size_t fLen = find.size();
		const size_t rLen = replace.size();
		for (size_t pos = 0; (pos = source.find(find, pos)) != std::string::npos; pos += rLen) {
			num++;
			source.replace(pos, fLen, replace);
		}

		return num;
	};

	inline uint32_t FNV_Hash(const void* key, const int len, uint32_t h) {
		// https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp
		h ^= 2166136261UL;
		const uint8_t* data = (const uint8_t*)key;
		for (int i = 0; i < len; ++i) {
			h ^= data[i];
			h *= 16777619;
		}
		return h;
	}

}