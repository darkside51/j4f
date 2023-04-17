#pragma once

#include "Platform_inc.h"

#ifdef j4f_PLATFORM_WINDOWS
// https://stackoverflow.com/questions/23258650/sleep1-and-sdl-delay1-takes-15-ms
// https://learn.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timebeginperiod
#include <windows.h>
#include <timeapi.h>

#pragma comment(lib, "winmm.lib")
#endif

namespace engine {

	void initPlatform() {
#ifdef j4f_PLATFORM_WINDOWS
		timeBeginPeriod(1u);
#endif
	}

	void deinitPlatform() {
#ifdef j4f_PLATFORM_WINDOWS
		timeEndPeriod(1u);
#endif
	}
}