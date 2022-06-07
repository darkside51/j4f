#pragma once

#include "../../Core/Math/math.h"

#include <string>
#include <vector>
#include <cstdint>

namespace gltf {
	struct Layout;
	struct Node;
	enum class AttributesSemantic : uint8_t;
}

namespace vulkan {
	struct VulkanBuffer;
}

namespace engine {

	struct Mesh_Geometry {
		struct Primitive {

		};
		std::vector<Primitive> primitives;
		uint16_t nodeIndex = 0xffff;
	};

	struct Mesh_Skin {
		uint16_t skeletonRoot = 0xffff;
		std::vector<glm::mat4> inverseBindMatrices;
		std::vector<uint16_t> joints;
	};

	struct Mesh_Animation {
		enum class Interpolation : uint8_t {
			LINEAR = 0,
			STEP = 1,
			CUBICSPLINE = 2
		};

		enum class AimationChannelPath : uint8_t {
			TRANSLATION = 0,
			ROTATION = 1,
			SCALE = 2,
			WEIGHTS = 3
		};

		struct AnimationSampler {
			std::vector<float> inputs;
			std::vector<glm::vec4> outputs;
			Interpolation interpolation = Interpolation::LINEAR;
		};

		struct AnimationChannel {
			uint16_t sampler;
			uint16_t target_node;
			AimationChannelPath path;
		};

		std::string name;
		std::vector<AnimationChannel> channels;
		std::vector<AnimationSampler> samplers;
		float start = std::numeric_limits<float>::max();
		float end = std::numeric_limits<float>::min();
		float duration;
		uint16_t minTargetNodeId = 0xffff;
		uint16_t maxTargetNodeId = 0;
	};

	struct Mesh_Data {
		struct MeshRenderParams {
			struct Layout {
				uint32_t firstIndex;	// номер первого индекса
				uint32_t indexCount;	// количество индексов
				size_t vbOffset;		// оффсет в вершинном буфере
				size_t ibOffset;		// оффсет в индексном буфере
				glm::vec3 minCorner;
				glm::vec3 maxCorner;
			};

			std::vector<Layout> layouts;
		};

		std::vector<uint16_t> sceneNodes;
		std::vector<gltf::Node> nodes;
		std::vector<Mesh_Geometry> meshes;
		std::vector<Mesh_Skin> skins;
		std::vector<MeshRenderParams> renderData;
		std::vector<Mesh_Animation> animations;

		std::vector<float> vertexBuffer;
		std::vector<uint32_t> indexBuffer;

		vulkan::VulkanBuffer* verticesBuffer = nullptr;
		vulkan::VulkanBuffer* indicesBuffer = nullptr;

		vulkan::VulkanBuffer* stage_vertices = nullptr;
		vulkan::VulkanBuffer* stage_indices = nullptr;
		size_t gpu_vbOffset = 0;
		size_t gpu_ibOffset = 0;

		uint32_t vertexSize = 0;
		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;

		// functions
		~Mesh_Data();

		void loadSkins(const gltf::Layout& layout);

		void loadNodes(const gltf::Layout& layout);

		void loadMeshes(const gltf::Layout& layout, const std::vector<gltf::AttributesSemantic>& allowedAttributes,
			const size_t vbOffset, const size_t ibOffset, const bool useOffsetsInRenderData);

		void loadAnimations(const gltf::Layout& layout);

		void initMeshNodeId(const gltf::Layout& layout, const uint16_t nodeId);

		void uploadGpuData(vulkan::VulkanBuffer*& vertices, vulkan::VulkanBuffer*& indices, const size_t vbOffset, const size_t ibOffset);

		void fillGpuData();

		inline void destroyBuffers() {
			vertexBuffer.clear();
			indexBuffer.clear();
		}
	};

}