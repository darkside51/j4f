#pragma once

#ifdef _DEBUG
	#ifdef j4f_PLATFORM_WINDOWS
		#include <intrin.h>
		#define ENGINE_BREAK __debugbreak();
		#define ENGINE_BREAK_CONDITION(x) if (!x) __debugbreak();
	#elif j4f_PLATFORM_LINUX
		#include <signal.h>
		#define ENGINE_BREAK raise(SIGTRAP);
		#define ENGINE_BREAK_CONDITION(x) if (!x) raise(SIGTRAP);
	#else
	#endif
#else
	#define ENGINE_BREAK
	#define ENGINE_BREAK_CONDITION(x)
#endif