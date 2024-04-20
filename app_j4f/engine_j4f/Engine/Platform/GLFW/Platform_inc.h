#pragma once

#include "../../File/CommonFileSystem.h"
#include "../../Graphics/Vulkan/vkRenderer.h"

//namespace vulkan {
//    class VulkanRenderer;
//}

namespace engine {
	using DefaultFileSystem = CommonFileSystem;
    using Renderer = vulkan::VulkanRenderer;

	void initPlatform();
	void deinitPlatform();
}