#pragma once

#include "../../Device/GLFW/GLFWDevice.h"
#include "../../File/CommonFileSystem.h"

//#define j4f_PLATFORM_WINDOWS

namespace engine {
	using Device = GLFWDevice;
	using DefaultFileSystem = CommonFileSystem;
}