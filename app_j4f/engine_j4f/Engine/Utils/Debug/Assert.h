#pragma once

#include "../../Log/Log.h"

#ifdef _DEBUG
	#ifdef j4f_PLATFORM_WINDOWS
		#include <intrin.h>
		#define ENGINE_BREAK __debugbreak();
	#elif j4f_PLATFORM_LINUX
		#include <signal.h>
		#define ENGINE_BREAK raise(SIGTRAP);
    #else
        #define ENGINE_BREAK
    #endif

    #define ENGINE_BREAK_CONDITION(x) if (!(x)) ENGINE_BREAK;
    #define ENGINE_BREAK_CONDITION_DO(x, tag, message, todo) if (!(x)) { ENGINE_BREAK;  \
        LOG_TAG_LEVEL(engine::LogLevel::L_ERROR, tag, "%s", message);                   \
        todo;                                                                           \
    }
#else
	#define ENGINE_BREAK
	#define ENGINE_BREAK_CONDITION(x)
    #define ENGINE_BREAK_CONDITION_DO(x, tag, message, todo) if (!(x)) {    \
        LOG_TAG_LEVEL(engine::LogLevel::L_ERROR, tag, "%s", message);       \
        todo;                                                               \
    }
#endif