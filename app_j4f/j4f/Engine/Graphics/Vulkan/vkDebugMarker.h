#pragma once

#include "vkDevice.h"
#include "../../Log/Log.h"

#include <vulkan/vulkan.h>
#include <array>

namespace vulkan::debugMarker {

	inline PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTag		= VK_NULL_HANDLE;
	inline PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectName		= VK_NULL_HANDLE;
	inline PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBegin				= VK_NULL_HANDLE;
	inline PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEnd					= VK_NULL_HANDLE;
	inline PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsert				= VK_NULL_HANDLE;

	inline VkDevice vkDevice = VK_NULL_HANDLE;

	inline void init(vulkan::VulkanDevice* device, std::vector<const char*>& extensions) {
		if (device->extensionSupported(VK_EXT_DEBUG_MARKER_EXTENSION_NAME)) {
			extensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
		}
	}

	inline void setup(vulkan::VulkanDevice* device) {
		if (device->extensionSupported(VK_EXT_DEBUG_MARKER_EXTENSION_NAME)) {
			// debug marker extension is not part of the core, so function pointers need to be loaded manually
			vkDebugMarkerSetObjectTag		= reinterpret_cast<PFN_vkDebugMarkerSetObjectTagEXT>(vkGetDeviceProcAddr(device->device,	"vkDebugMarkerSetObjectTagEXT"));
			vkDebugMarkerSetObjectName		= reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(vkGetDeviceProcAddr(device->device,	"vkDebugMarkerSetObjectNameEXT"));
			vkCmdDebugMarkerBegin			= reinterpret_cast<PFN_vkCmdDebugMarkerBeginEXT>(vkGetDeviceProcAddr(device->device,		"vkCmdDebugMarkerBeginEXT"));
			vkCmdDebugMarkerEnd				= reinterpret_cast<PFN_vkCmdDebugMarkerEndEXT>(vkGetDeviceProcAddr(device->device,			"vkCmdDebugMarkerEndEXT"));
			vkCmdDebugMarkerInsert			= reinterpret_cast<PFN_vkCmdDebugMarkerInsertEXT>(vkGetDeviceProcAddr(device->device,		"vkCmdDebugMarkerInsertEXT"));

			if (vkDebugMarkerSetObjectName != VK_NULL_HANDLE) {
				vkDevice = device->device;
				LOG_TAG_LEVEL(engine::LogLevel::L_CUSTOM, VULKAN_DEBUG_MARKER, "is ON");
			} else {
				LOG_TAG_LEVEL(engine::LogLevel::L_CUSTOM, VULKAN_DEBUG_MARKER, "is NO get functions");
			}
		} else {
			LOG_TAG_LEVEL(engine::LogLevel::L_CUSTOM, VULKAN_DEBUG_MARKER, "is NO awailable");
		}
	}

	// sets the debug name of an object
	// all Objects in vulkan are represented by their 64-bit handles which are passed into this function
	// along with the object type
	inline void setObjectName(uint64_t object, VkDebugReportObjectTypeEXT objectType, const char* name) {
		// check for valid function pointer (may not be present if not running in a debugging application)
		if (vkDevice) {
			VkDebugMarkerObjectNameInfoEXT nameInfo = {};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = objectType;
			nameInfo.object = object;
			nameInfo.pObjectName = name;
			vkDebugMarkerSetObjectName(vkDevice, &nameInfo);
		}
	}

	// set the tag for an object
	inline void setObjectTag(uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag) {
		if (vkDevice) {
			VkDebugMarkerObjectTagInfoEXT tagInfo = {};
			tagInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT;
			tagInfo.objectType = objectType;
			tagInfo.object = object;
			tagInfo.tagName = name;
			tagInfo.tagSize = tagSize;
			tagInfo.pTag = tag;
			vkDebugMarkerSetObjectTag(vkDevice, &tagInfo);
		}
	}

	// start a new debug marker region
	inline void beginRegion(VkCommandBuffer cmdbuffer, const char* markerName, const std::array<float, 4>& color) {
		if (vkDevice) {
			VkDebugMarkerMarkerInfoEXT markerInfo = {};
			markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
			memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
			markerInfo.pMarkerName = markerName;
			vkCmdDebugMarkerBegin(cmdbuffer, &markerInfo);
		}
	}

	// insert a new debug marker into the command buffer
	inline void insert(VkCommandBuffer cmdbuffer, const char* markerName, const std::array<float, 4>& color) {
		if (vkDevice) {
			VkDebugMarkerMarkerInfoEXT markerInfo = {};
			markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
			memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
			markerInfo.pMarkerName = markerName;
			vkCmdDebugMarkerInsert(cmdbuffer, &markerInfo);
		}
	}

	// end the current debug marker region
	inline void endRegion(VkCommandBuffer cmdBuffer) {
		// check for valid function (may not be present if not running in a debugging application)
		if (vkCmdDebugMarkerEnd) {
			vkCmdDebugMarkerEnd(cmdBuffer);
		}
	}
}

#ifdef GPU_DEBUG_MARKER_ENABLED
#define GPU_DEBUG_MARKERS
#define GPU_DEBUG_MARKERS_INIT(device, extensions) vulkan::debugMarker::init(device, extensions)
#define GPU_DEBUG_MARKERS_SETUP(device) vulkan::debugMarker::setup(device)
#define	GPU_DEBUG_MARKER_SET_OBJECT_NAME(object, objectType, name) vulkan::debugMarker::setObjectName((uint64_t)object, objectType, name)
#define	GPU_DEBUG_MARKER_SET_OBJECT_TAG(object, objectType, name, tagSize, tag) vulkan::debugMarker::setObjectTag((uint64_t)object, objectType, name, tagSize, tag)
#define	GPU_DEBUG_MARKER_BEGIN_REGION(cmdbuffer, markerName, r, g, b, a) vulkan::debugMarker::beginRegion(cmdbuffer, markerName, {r, g, b, a})
#define	GPU_DEBUG_MARKER_INSERT(cmdbuffer, markerName, r, g, b, a) vulkan::debugMarker::insert(cmdbuffer, markerName, {r, g, b, a})
#define	GPU_DEBUG_MARKER_END_REGION(cmdbuffer) vulkan::debugMarker::endRegion(cmdbuffer)
#else
#define GPU_DEBUG_MARKERS_INIT(device, extensions)
#define GPU_DEBUG_MARKERS_SETUP(device) 
#define	GPU_DEBUG_MARKER_SET_OBJECT_NAME(object, objectType, name) 
#define	GPU_DEBUG_MARKER_SET_OBJECT_TAG(object, objectType, name, tagSize, tag) 
#define	GPU_DEBUG_MARKER_BEGIN_REGION(cmdbuffer, markerName, r, g, b, a) 
#define	GPU_DEBUG_MARKER_INSERT(cmdbuffer, markerName, r, g, b, a) 
#define	GPU_DEBUG_MARKER_END_REGION(cmdbuffer) 
#endif // DEBUG