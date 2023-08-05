#pragma once

#include "../../Device/GLFW/GLFWDevice.h"
#include "../../File/CommonFileSystem.h"
#include "../../Graphics/Vulkan/vkRenderer.h"

//namespace vulkan {
//    class VulkanRenderer;
//}

namespace engine {
	using Device = GLFWDevice;
	using DefaultFileSystem = CommonFileSystem;
    using Renderer = vulkan::VulkanRenderer;

	void initPlatform();
	void deinitPlatform();
}