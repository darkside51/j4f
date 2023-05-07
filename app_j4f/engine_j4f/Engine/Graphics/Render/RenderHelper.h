#pragma once

#include "../Vulkan/vkBuffer.h"
#include "../Vulkan/vkPipeline.h"
#include "../Vulkan/vkCommandBuffer.h"

#include "../../Core/Math/mathematic.h"

#include "CommonVertexTypes.h"

#include <vector>
#include <cstdint>

namespace vulkan {
	class VulkanRenderer;
	struct RenderData;
	struct GPUParamLayoutInfo;
}

namespace engine {

	class GpuProgramsManager;
	class AutoBatchRenderer;

	struct VulkanStreamBuffer {
		vulkan::VulkanBuffer vkBuffer;
		VulkanStreamBuffer* next = nullptr;
		VulkanStreamBuffer* current;
		VkBufferUsageFlags usageFlags = 0;
		size_t offset;
		size_t size;

		VulkanStreamBuffer() : current(this), offset(0), size(0) {}

		void createBuffer(const size_t sz);

		~VulkanStreamBuffer() {
			destroy();
		}

		inline void destroy() {
			vkBuffer.destroy();
			if (next) {
				delete next;
				next = nullptr;
			}
			offset = 0;
		}

		void recreate(const VkBufferUsageFlags flags) {
			usageFlags = flags;
			offset = 0;

			if (next == nullptr) {
				return;
			}

			size_t sz = 0;
			current = this;
			while (current) {
				sz += current->size;
				current = current->next;
			}
			current = this;

			destroy();
			createBuffer(sz);
		}

		inline void upload(const void* data, const size_t sz) {
			vkBuffer.upload(data, sz, offset, false);
			offset += sz;
		}

		inline const vulkan::VulkanBuffer& addData(const void* data, const size_t sz, size_t& out_offset) {
			if (!vkBuffer.isValid()) createBuffer(sz);

			if (current->vkBuffer.m_size >= (current->offset + sz)) {
				out_offset = current->offset;
				current->upload(data, sz);
			} else {
				out_offset = 0;
				current->next = new VulkanStreamBuffer();
				current->next->usageFlags = current->usageFlags;
				current->next->createBuffer(sz);
				current = current->next;
				current->upload(data, sz);
			}

			return current->vkBuffer;
		}
	};

	enum class CommonPipelines : uint8_t {
		COMMON_PIPELINE_LINES = 0,
		COMMON_PIPELINE_COLORED_PLAINS = 1,
		COMMON_PIPELINE_TEXTURED = 2,
		COMMON_PIPELINE_TEXTURED_DEPTH_RW = 3,
		COMMON_PIPELINES_COUNT
	};

	class RenderHelper {
	public:
		RenderHelper(vulkan::VulkanRenderer* r, GpuProgramsManager* m);
		~RenderHelper();

		void updateFrame();
		void onResize();

		const vulkan::VulkanBuffer& addDynamicVerteces(const void* data, const size_t sz, size_t& out_offset);
		const vulkan::VulkanBuffer& addDynamicIndices(const void* data, const size_t sz, size_t& out_offset);

		inline AutoBatchRenderer* getAutoBatchRenderer() { return _autoBatchRenderer; }

		const vulkan::VulkanPipeline* getPipeline(const CommonPipelines pipeline) const {
			return _commonPipelines[static_cast<uint8_t>(pipeline)];
		}

		void initCommonPipelines();

		///////
		void drawBoundingBox(const vec3f& c1, const vec3f& c2, const mat4f& cameraMatrix, const mat4f& worldMatrix, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const bool batch = true);
		void drawSphere(const vec3f& c, const float r, const mat4f& cameraMatrix, const mat4f& worldMatrix, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const bool batch = true);
	private:
		
		vulkan::VulkanRenderer* _renderer;
		GpuProgramsManager* _gpuProgramManager;
		AutoBatchRenderer* _autoBatchRenderer;

		vulkan::RenderData* _debugDrawRenderData = nullptr;

		std::vector<VulkanStreamBuffer> _dynamic_vertices;
		std::vector<VulkanStreamBuffer> _dynamic_indices;
		vulkan::VulkanPipeline* _commonPipelines[static_cast<uint8_t>(CommonPipelines::COMMON_PIPELINES_COUNT)];
	};
}