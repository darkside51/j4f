#pragma once

#include <Platform_inc.h>
#include "../Utils/StringHelper.h"
#include "../Time/Time.h"
#include <inttypes.h>

#ifdef j4f_PLATFORM_WINDOWS
	#include <windows.h>
	#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

namespace engine {
	template <typename...Args>
	inline void printLog(const char* fmt, Args&&...args) {
		//OutputDebugString(fmt_string(fmt, std::forward<Args>(args)...));
		printf(fmt, std::forward<Args>(args)...);
	}
}
#else
	#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

namespace engine {
	template <typename...Args>
	inline void printLog(const char* fmt, Args&&...args) {
		printf(fmt, std::forward<Args>(args)...);
	}
}
#endif

namespace engine {

	template <typename...Args>
	inline void log(const char* fmt, Args&&...args) {
		const char* f1 = fmt_string("(%" PRIu64 ") %s \n", unixUTCTime(), fmt); // good variant?
		printLog(f1, std::forward<Args>(args)...);
	}

	template <typename...Args>
	inline void log_tag(const char* tag, const char* fmt, Args&&...args) {
		const char* f1 = fmt_string("(%" PRIu64 ") (%s) %s \n", unixUTCTime(), tag, fmt);
		printLog(f1, std::forward<Args>(args)...);
	}
}

#define STR(x) #x
#define LOG_TAG(tag, fmt, ...) engine::log_tag(STR(::tag::), fmt, __VA_ARGS__)
#define LOG_FILE(fmt, ...) engine::log_tag(__FILENAME__, fmt, __VA_ARGS__)
#define LOG_LINE(fmt, ...) engine::log_tag(fmt_string("%s::%d", __FILENAME__, __LINE__), fmt, __VA_ARGS__)