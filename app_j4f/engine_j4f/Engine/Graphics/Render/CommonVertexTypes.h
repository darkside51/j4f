#pragma once

#include "../../Core/Common.h"

#include <vulkan/vulkan.h>
#include <vector>

namespace engine {

	struct ColoredVertex {
		float position[3];
		float color[3];

		inline static std::vector<VkVertexInputAttributeDescription> getVertexAttributesDescription() {
			std::vector<VkVertexInputAttributeDescription> vertexInputAttributs(2);
			// attribute location 0: position
			vertexInputAttributs[0].binding = 0;
			vertexInputAttributs[0].location = 0;
			vertexInputAttributs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			vertexInputAttributs[0].offset = offset_of(ColoredVertex, position);
			// attribute location 1: color
			vertexInputAttributs[1].binding = 0;
			vertexInputAttributs[1].location = 1;
			vertexInputAttributs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			vertexInputAttributs[1].offset = offset_of(ColoredVertex, color);

			return vertexInputAttributs;
		}
	};

	struct TexturedVertex {
		float position[3];
		float uv[2];

		inline static std::vector<VkVertexInputAttributeDescription> getVertexAttributesDescription() {
			std::vector<VkVertexInputAttributeDescription> vertexInputAttributs(2);
			// attribute location 0: position
			vertexInputAttributs[0].binding = 0;
			vertexInputAttributs[0].location = 0;
			vertexInputAttributs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			vertexInputAttributs[0].offset = offset_of(TexturedVertex, position);
			// attribute location 1: uv
			vertexInputAttributs[1].binding = 0;
			vertexInputAttributs[1].location = 1;
			vertexInputAttributs[1].format = VK_FORMAT_R32G32_SFLOAT;
			vertexInputAttributs[1].offset = offset_of(TexturedVertex, uv);

			return vertexInputAttributs;
		}
	};

	struct ColoredTexturedVertex {
		float position[3];
		float color[3];
		float uv[2];

		inline static std::vector<VkVertexInputAttributeDescription> getVertexAttributesDescription() {
			std::vector<VkVertexInputAttributeDescription> vertexInputAttributs(3);
			// attribute location 0: position
			vertexInputAttributs[0].binding = 0;
			vertexInputAttributs[0].location = 0;
			vertexInputAttributs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			vertexInputAttributs[0].offset = offset_of(ColoredTexturedVertex, position);
			// attribute location 1: color
			vertexInputAttributs[1].binding = 0;
			vertexInputAttributs[1].location = 1;
			vertexInputAttributs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			vertexInputAttributs[1].offset = offset_of(ColoredTexturedVertex, color);
			// attribute location 2: uv
			vertexInputAttributs[2].binding = 0;
			vertexInputAttributs[2].location = 2;
			vertexInputAttributs[2].format = VK_FORMAT_R32G32_SFLOAT;
			vertexInputAttributs[2].offset = offset_of(ColoredTexturedVertex, uv);

			return vertexInputAttributs;
		}
	};

}