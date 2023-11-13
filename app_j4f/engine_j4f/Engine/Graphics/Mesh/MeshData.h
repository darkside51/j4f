#pragma once

#include "../../Core/Math/mathematic.h"
#include "../../Core/Ref_ptr.h"

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

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
		//struct Primitive {
		//
		//};
		//std::vector<Primitive> primitives;
		uint16_t nodeIndex = 0xffffu;
	};

	struct Mesh_Skin {
		uint16_t skeletonRoot = 0xffffu;
		std::vector<mat4f> inverseBindMatrices;
		std::vector<uint16_t> joints;
	};

	struct Mesh_Animation {
		enum class Interpolation : uint8_t {
			LINEAR = 0u,
			STEP = 1u,
			CUBICSPLINE = 2u
		};

		enum class AimationChannelPath : uint8_t {
			TRANSLATION = 0u,
			ROTATION = 1u,
			SCALE = 2u,
			WEIGHTS = 3u
		};

		struct AnimationSampler {
			std::vector<float> inputs;
			std::vector<vec4f> outputs;
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
		float duration = 0.0f;
		uint16_t minTargetNodeId = 0xffffu;
		uint16_t maxTargetNodeId = 0u;
	};

	struct Mesh_Data {
		struct MeshRenderParams {
			struct Layout {
				uint8_t primitiveMode;
				uint32_t firstIndex;	// номер первого индекса
				uint32_t indexCount;	// количество индексов
				size_t vbOffset;		// оффсет в вершинном буфере
				size_t ibOffset;		// оффсет в индексном буфере
				vec3f minCorner;
				vec3f maxCorner;
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

		ref_ptr<vulkan::VulkanBuffer> verticesBuffer = nullptr;
		ref_ptr<vulkan::VulkanBuffer> indicesBuffer = nullptr;

		std::unique_ptr<vulkan::VulkanBuffer> stage_vertices = nullptr;
		std::unique_ptr<vulkan::VulkanBuffer> stage_indices = nullptr;

		size_t gpu_vbOffset = 0u;
		size_t gpu_ibOffset = 0u;

		uint32_t vertexSize = 0u;
		uint32_t vertexCount = 0u;
		uint32_t indexCount = 0u;

		// functions
		~Mesh_Data() = default;

		void loadSkins(const gltf::Layout& layout);

		void loadNodes(const gltf::Layout& layout);

		size_t loadMeshes(const gltf::Layout& layout, const std::vector<gltf::AttributesSemantic>& allowedAttributes,
			size_t& vbOffset, const size_t ibOffset, const bool useOffsetsInRenderData);

		void loadAnimations(const gltf::Layout& layout);

		void initMeshNodeId(const gltf::Layout& layout, const uint16_t nodeId);

		void uploadGpuData(std::unique_ptr<vulkan::VulkanBuffer>& vertices, std::unique_ptr<vulkan::VulkanBuffer>& indices, const size_t vbOffset, const size_t ibOffset);

		void fillGpuData();

		inline void destroyBuffers() {
			vertexBuffer.clear();
			indexBuffer.clear();
		}
	};

}