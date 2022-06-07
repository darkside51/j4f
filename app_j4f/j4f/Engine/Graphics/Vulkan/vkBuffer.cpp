#include "vkBuffer.h"
#include "vkDevice.h"

#include <assert.h>

namespace vulkan {

	void VulkanDeviceBuffer::createStageBuffer(VulkanDevice* device, const VkSharingMode sharingMode, const VkDeviceSize size) {
		if (m_stageBuffer.isValid()) {
			assert(false); // "stage buffer already exists!"
		}

		device->createBuffer(
			sharingMode,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_stageBuffer,
			size
		);
	}
}