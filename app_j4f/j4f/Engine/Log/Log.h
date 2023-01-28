#pragma once

#include <Platform_inc.h>
#include "../Utils/StringHelper.h"
#include "../Time/Time.h"
#include "../Core/EngineModule.h"
#include <inttypes.h>

#ifdef j4f_PLATFORM_WINDOWS
	#include <windows.h>
	#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING  0x0004
#endif

static HANDLE stdoutHandle = nullptr;
static DWORD outModeInit = 0;

inline void setupConsole() {
 	DWORD outMode = 0;
	stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (stdoutHandle == INVALID_HANDLE_VALUE) {
		exit(GetLastError());
	}

	if (!GetConsoleMode(stdoutHandle, &outMode)) {
		exit(GetLastError());
	}

	outModeInit = outMode;

	// enable ANSI escape codes
	outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(stdoutHandle, outMode)) {
		exit(GetLastError());
	}
}

inline void restoreConsole() {
	// reset console mode
	if (!SetConsoleMode(stdoutHandle, outModeInit)) {
		exit(GetLastError());
	 }
 }

//#define USE_NOSTD_CONSOLE_OUTPUT

namespace engine {
	template <typename...Args>
	inline void printLog(const char* fmt, Args&&...args) {
#ifdef USE_NOSTD_CONSOLE_OUTPUT
		OutputDebugString(fmt_string(fmt, std::forward<Args>(args)...)); // USE_MSVC_CONSOLE_OUTPUT
#else
		//system("Color 0A");
		printf(fmt, std::forward<Args>(args)...);
		//fmt::print(fmt, std::forward<Args>(args)...);
#endif // USE_NOSTD_CONSOLE_OUTPUT
	}
}
#else
	#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

inline void setupConsole() {}
inline void restoreConsole() {}

namespace engine {
	template <typename...Args>
	inline void printLog(const char* fmt, Args&&...args) {
		printf(fmt, std::forward<Args>(args)...);
	}
}
#endif

namespace engine {
	class LogManager : public IEngineModule {
	public:
		LogManager() {
			setupConsole();
		}

		~LogManager() {
			restoreConsole();
		}
	};

	enum class LogLevel : uint8_t {
		L_COMMON	= 0,
		L_MESSAGE	= 1,
		L_DEBUG		= 2,
		L_ERROR		= 3,
		L_CUSTOM	= 4
	};

	template <typename...Args>
	inline void log(const char* fmt, Args&&...args) {
		const char* f1 = fmt_string("({}) {} \n", unixUTCTime(), fmt); // good variant?
		printLog(f1, std::forward<Args>(args)...);
	}

	template <typename...Args>
	inline void log_tag(const char* tagColor, const char* tag, const char* fmt, Args&&...args) {
		
#ifdef USE_NOSTD_CONSOLE_OUTPUT
		//const char* f1 = fmt_string("(%" PRIu64 ") (%s) %s \n", unixUTCTime(), tag, fmt);
		const char* f1 = fmt_string("({}) ({}) {} \n", unixUTCTime(), tag, fmt);
#else
		const char* blackColor = "\033[0m";
		const char* f1 = fmt_string("({}) ({}{}{}) {} \n", unixUTCTime(), tagColor, tag, blackColor, fmt);
#endif
		
		printLog(f1, std::forward<Args>(args)...);
	}

	template <typename...Args>
	inline void log_tag_color(const LogLevel l, const char* tag, const char* fmt, Args&&...args) {
		// https://stackoverflow.com/questions/4053837/colorizing-text-in-the-console-with-c
		const char* tagColor;
		switch (l) {
			case LogLevel::L_MESSAGE:
				tagColor = "\x1B[32m"; // green
				break;
			case LogLevel::L_ERROR:
				tagColor = "\x1B[31m"; // red
				break;
			case LogLevel::L_DEBUG:
				tagColor = "\x1B[33m"; // yellow
				break;
			case LogLevel::L_CUSTOM:
				tagColor = "\x1B[36m"; // cyan
				break;
			default:
				tagColor = "\033[0m"; // default
				break;
		}
		log_tag(tagColor, tag, fmt, std::forward<Args>(args)...);
	}
}

#define STR(x) #x
#define LOG_TAG(tag, ...) engine::log_tag_color(engine::LogLevel::L_MESSAGE, STR(::tag::), __VA_ARGS__)
#define LOG_FILE(fmt, ...) engine::log_tag_color(engine::LogLevel::L_MESSAGE, __FILENAME__, fmt, __VA_ARGS__)
#define LOG_LINE(fmt, ...) engine::log_tag_color(engine::LogLevel::L_MESSAGE, fmt_string("{}::{}", __FILENAME__, __LINE__), fmt, __VA_ARGS__)
//#define LOG_LINE(fmt, ...) engine::log_tag_color(engine::LogLevel::L_MESSAGE, fmt_string("%s::%s", __FILENAME__, __LINE__), fmt, __VA_ARGS__)

#define LOG_TAG_LEVEL(level, tag, ...) engine::log_tag_color(level, STR(::tag::), __VA_ARGS__)
#define LOG_FILE_LEVEL(level, ...) engine::log_tag_color(level, __FILENAME__, __VA_ARGS__)
#define LOG_LINE_LEVEL(level, ...) engine::log_tag_color(level, fmt_string("{}::{}", __FILENAME__, __LINE__), __VA_ARGS__)
//#define LOG_LINE_LEVEL(level, fmt, ...) engine::log_tag_color(level, fmt_string("%s::%s", __FILENAME__, __LINE__), fmt, __VA_ARGS__)