#pragma once

#ifdef _DEBUG

#ifdef j4f_PLATFORM_WINDOWS

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

// todo: how to fix placement new?
#define debug_new new(_NORMAL_BLOCK, __FILE__, __LINE__)
//#define new debug_new // uncomment this to enable leaks normal paths output

// replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type

namespace engine {
    inline void enableMemoryLeaksDebugger() { _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); }
}

#define SKIP_DEBUG_MEMORY_LEAKS(x)

#else // j4f_PLATFORM_WINDOWS

#include <cstdint>

//#define debug_new new(__FILE__, __LINE__)

//for g++, "new" is expanded only once. The key is the "&& 0" which makes it false and causes the real new to be used. For example,
//#define new (__file__=__FILE__, __line__=__LINE__) && 0 ? NULL : new

extern thread_local const char* __file__;
extern thread_local uint32_t __line__;
extern thread_local bool __skip_check_memory__;

#ifndef debug_new
//#define debug_new (__file__=__FILE__, __line__=__LINE__) && 0 ? NULL : new
#define debug_new (__file__=__FILE__) && (__line__=__LINE__) && 0 ? nullptr : new
//#define new debug_new // - can use this on target cpp file
#endif

#define SKIP_DEBUG_MEMORY_LEAKS(x) __skip_check_memory__ = x;

namespace engine {
    inline void enableMemoryLeaksDebugger() noexcept {}

    inline class MemoryChecker {
    public:
        ~MemoryChecker() {
            checkLeaks();
        }
    private:
        void checkLeaks() const;
    } memoryChecker; // must be a first allocation
}
#endif // j4f_PLATFORM_WINDOWS

#else
#define SKIP_DEBUG_MEMORY_LEAKS(x)

#endif // _DEBUG
