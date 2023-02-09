#pragma once

#ifdef _DEBUG

#ifdef j4f_PLATFORM_WINDOWS

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

// todo: how to fix emplacement new?
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
//#define new DEBUG_NEW // uncomment this to enable leaks normal paths output

// replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type

namespace engine {
	inline void enableMemoryLeaksDebugger() { _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); }
}
#else // j4f_PLATFORM_WINDOWS
namespace engine {
	inline void enableMemoryLeaksDebugger() {}
}
#endif // j4f_PLATFORM_WINDOWS


#endif // _DEBUG
