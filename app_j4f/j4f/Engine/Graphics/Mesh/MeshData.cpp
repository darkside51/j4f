#include "MeshData.h"

#include "Loader_gltf.h"

#include "../../Core/Engine.h"
#include "../Graphics.h"
#include "../Vulkan/vkRenderer.h"
#include "../Vulkan/vkBuffer.h"

#include <iterator>

namespace engine {

	Mesh_Data::~Mesh_Data() {
		if (stage_vertices) {
			delete stage_vertices;
			stage_vertices = nullptr;
		}

		if (stage_indices) {
			delete stage_indices;
			stage_indices = nullptr;
		}
	};

	void Mesh_Data::loadMeshes(const gltf::Layout& layout, const std::vector<gltf::AttributesSemantic>& allowedAttributes,
		const size_t vbOffset, const size_t ibOffset, const bool useOffsetsInRenderData) {

		auto&& gltf_meshes = layout.meshes;
		if (!gltf_meshes.empty()) {
			const size_t meshesCount = gltf_meshes.size();
			meshes.reserve(meshesCount);
			renderData.reserve(meshesCount);

			const uint8_t semanticsCount = static_cast<uint8_t>(gltf::AttributesSemantic::SEMANTICS_COUNT);
			const uint8_t allowedAttributsCount = allowedAttributes.size();
			std::vector<const float*> buffers(semanticsCount);
			std::vector<uint8_t> buffersDimensions(semanticsCount);
			std::vector<uint8_t> allowedAttributesFound(allowedAttributsCount);

			uint32_t commonVertexCount = 0;

			for (auto&& mesh : gltf_meshes) {
				Mesh_Geometry mmesh;
				meshes.push_back(mmesh);
				MeshRenderParams render_data;

				for (auto&& primitive : mesh.primitives) {
					const uint32_t firstIndex = static_cast<uint32_t>(indexBuffer.size());
					const uint32_t firstVertex = static_cast<uint32_t>(vertexBuffer.size());

					uint32_t mesh_vertexCount = 0;
					uint32_t mesh_indexCount = 0;
					uint32_t mesh_vertexSize = 0;
					uint8_t attributesCount = 0;

					for (uint8_t i = 0; i < semanticsCount; ++i) {
						buffers[i] = nullptr;
						if (i < allowedAttributsCount) { allowedAttributesFound[i] = 0; }
					}

					// vertex attributes
					glm::vec3 minCorner;
					glm::vec3 maxCorner;

					for (auto it = primitive.attributes.begin(); it != primitive.attributes.end(); ++it) {
						if (!allowedAttributes.empty()) {
							auto attrIt = std::find(allowedAttributes.begin(), allowedAttributes.end(), it->first);
							if (attrIt == allowedAttributes.end()) continue;
							const auto distance = std::distance(allowedAttributes.begin(), attrIt);
							allowedAttributesFound[distance] = 1;
						}

						++attributesCount;
						const gltf::Accessor& accessor = layout.accessors[it->second];
						const gltf::BufferView& view = layout.bufferViews[accessor.bufferView];
						buffers[static_cast<uint8_t>(it->first)] = reinterpret_cast<const float*>(&(layout.buffers[view.buffer].data[accessor.offset + view.offset]));

						if (it->first == gltf::AttributesSemantic::POSITION) {
							mesh_vertexCount = accessor.count;
							minCorner = glm::vec3(accessor.min[0], accessor.min[1], accessor.min[2]);
							maxCorner = glm::vec3(accessor.max[0], accessor.max[1], accessor.max[2]);
						}

						switch (accessor.type) {
							case gltf::AccessorType::VEC2:
								buffersDimensions[static_cast<uint8_t>(it->first)] = 2;
								mesh_vertexSize += 2;
									break;
							case gltf::AccessorType::VEC3:
								buffersDimensions[static_cast<uint8_t>(it->first)] = 3;
								mesh_vertexSize += 3;
									break;
							case gltf::AccessorType::VEC4:
								buffersDimensions[static_cast<uint8_t>(it->first)] = 4;
								mesh_vertexSize += 4;
									break;
							default:
								break;
						}
					}

					if ((allowedAttributsCount != 0) && (attributesCount != allowedAttributsCount)) {
						for (uint8_t i = 0; i < allowedAttributsCount; ++i) {
							if (allowedAttributesFound[i] == 0) {
								const uint8_t a_idx = static_cast<uint8_t>(allowedAttributes[i]);
								switch (allowedAttributes[i]) {
									case gltf::AttributesSemantic::JOINTS:
									case gltf::AttributesSemantic::WEIGHT:
									{
										mesh_vertexSize += 4;
										buffersDimensions[a_idx] = 4;
									}
										break;
									case gltf::AttributesSemantic::TEXCOORD_0:
									{
										mesh_vertexSize += 2;
										buffersDimensions[a_idx] = 2;
									}
									break;
									default:
										break;
								}
							}
						}
					}

					vertexBuffer.resize(firstVertex + mesh_vertexSize * mesh_vertexCount);

					uint32_t idx = 0;

					if (allowedAttributsCount != 0) { // use only allowed attributes
						for (size_t v = 0; v < mesh_vertexCount; ++v) {
							for (uint8_t i = 0; i < allowedAttributsCount; ++i) {
								const uint8_t a_idx = static_cast<uint8_t>(allowedAttributes[i]);
								const uint32_t dataSize = buffersDimensions[a_idx];

								if (const float* buffer = buffers[a_idx]) {
									if (a_idx == static_cast<uint8_t>(gltf::AttributesSemantic::JOINTS)) {
										const uint16_t* jointIndicesBuffer = reinterpret_cast<const uint16_t*>(buffer);
										vertexBuffer[firstVertex + idx + 0] = jointIndicesBuffer[v * 4 + 0];
										vertexBuffer[firstVertex + idx + 1] = jointIndicesBuffer[v * 4 + 1];
										vertexBuffer[firstVertex + idx + 2] = jointIndicesBuffer[v * 4 + 2];
										vertexBuffer[firstVertex + idx + 3] = jointIndicesBuffer[v * 4 + 3];
									} else {
										memcpy(&vertexBuffer[firstVertex + idx], &buffer[v * dataSize], dataSize * sizeof(float));
									}
								} else {
									for (uint32_t c = 0; c < dataSize; ++c) {
										vertexBuffer[firstVertex + idx + c] = 0.0f;
									}
								}

								idx += dataSize;
							}
						}
					} else {
						for (size_t v = 0; v < mesh_vertexCount; ++v) {
							for (uint8_t i = 0; i < semanticsCount; ++i) {
								if (const float* buffer = buffers[i]) {
									const uint32_t dataSize = buffersDimensions[i];
									if (i == static_cast<uint8_t>(gltf::AttributesSemantic::JOINTS)) {
										const uint16_t* jointIndicesBuffer = reinterpret_cast<const uint16_t*>(buffer);
										vertexBuffer[firstVertex + idx + 0] = jointIndicesBuffer[v * 4 + 0];
										vertexBuffer[firstVertex + idx + 1] = jointIndicesBuffer[v * 4 + 1];
										vertexBuffer[firstVertex + idx + 2] = jointIndicesBuffer[v * 4 + 2];
										vertexBuffer[firstVertex + idx + 3] = jointIndicesBuffer[v * 4 + 3];
									} else {
										memcpy(&vertexBuffer[firstVertex + idx], &buffer[v * dataSize], dataSize * sizeof(float));
									}
									idx += dataSize;
								}
							}
						}
					}

					// indexes
					const gltf::Accessor& accessor = layout.accessors[primitive.indices];
					const gltf::BufferView& bufferView = layout.bufferViews[accessor.bufferView];
					const gltf::Buffer& buffer = layout.buffers[bufferView.buffer];
					mesh_indexCount = accessor.count;

					indexBuffer.resize(firstIndex + mesh_indexCount);

					const uint32_t startVertex = (useOffsetsInRenderData ? 0 : (vbOffset / (mesh_vertexSize * sizeof(float))));
					const uint32_t startIndex = (useOffsetsInRenderData ? 0 : (ibOffset / (sizeof(uint32_t))));

					switch (accessor.componentType) {
						case gltf::AccessorComponentType::UNSIGNED_INT:
							for (uint32_t index = 0; index < mesh_indexCount; ++index) {
								const size_t offset = accessor.offset + bufferView.offset + index * sizeof(uint32_t);
								const uint8_t v1 = buffer.data[offset + 0];
								const uint8_t v2 = buffer.data[offset + 1];
								const uint8_t v3 = buffer.data[offset + 2];
								const uint8_t v4 = buffer.data[offset + 3];
								const uint32_t v = (static_cast<uint32_t>(v1) << 0) | (static_cast<uint32_t>(v2) << 8) | (static_cast<uint32_t>(v3) << 16) | (static_cast<uint32_t>(v4) << 24);
								indexBuffer[firstIndex + index] = startVertex + commonVertexCount + v;
							}
								break;
						case gltf::AccessorComponentType::UNSIGNED_SHORT:
							for (uint32_t index = 0; index < mesh_indexCount; ++index) {
								const size_t offset = accessor.offset + bufferView.offset + index * sizeof(uint16_t);
								const uint8_t v1 = buffer.data[offset + 0];
								const uint8_t v2 = buffer.data[offset + 1];
								const uint16_t v = (static_cast<uint16_t>(v1) << 0) | (static_cast<uint16_t>(v2) << 8);
								indexBuffer[firstIndex + index] = startVertex + commonVertexCount + v;
							}
								break;
						case gltf::AccessorComponentType::UNSIGNED_BYTE:
							for (uint32_t index = 0; index < mesh_indexCount; ++index) {
								indexBuffer[firstIndex + index] = startVertex + commonVertexCount + buffer.data[accessor.offset + bufferView.offset + index];
							}
							break;
						default:
							break;
					}

					commonVertexCount += mesh_vertexCount;

					if (vertexSize == 0) {
						vertexSize = mesh_vertexSize;
					}

					render_data.layouts.emplace_back(Mesh_Data::MeshRenderParams::Layout{
							firstIndex + startIndex,					// firstIndex
							mesh_indexCount,							// indexCount
							(useOffsetsInRenderData ? vbOffset : 0),	// vbOffset
							(useOffsetsInRenderData ? ibOffset : 0),	// ibOffset
							minCorner,
							maxCorner
						});
				}

				renderData.push_back(render_data);
			}

			indexCount = static_cast<uint32_t>(indexBuffer.size());
			vertexCount = commonVertexCount;
		}
	}

	void Mesh_Data::initMeshNodeId(const gltf::Layout& layout, const uint16_t nodeId) {
		const gltf::Node& node = layout.nodes[nodeId];

		if (node.mesh != 0xffff) {
			meshes[node.mesh].nodeIndex = nodeId;
		}

		for (const uint16_t cNodeId : node.children) {
			initMeshNodeId(layout, cNodeId);
		}
	}

	void Mesh_Data::loadNodes(const gltf::Layout& layout) {
		sceneNodes = layout.scenes[layout.scene].nodes;
		nodes = layout.nodes; // copy data

		for (const uint16_t nodeId : sceneNodes) {
			initMeshNodeId(layout, nodeId);
		}
	}

	void Mesh_Data::loadSkins(const gltf::Layout& layout) {
		const size_t skinsSize = layout.skins.size();
		skins.resize(skinsSize);

		for (size_t i = 0; i < skinsSize; ++i) {
			const gltf::Skin& gltf_skin = layout.skins[i];
			skins[i].skeletonRoot = gltf_skin.skeleton;
			skins[i].joints = gltf_skin.joints; // copy data

			if (gltf_skin.inverseBindMatrices != 0xffff) {
				const gltf::Accessor& accessor = layout.accessors[gltf_skin.inverseBindMatrices];
				const gltf::BufferView& bufferView = layout.bufferViews[accessor.bufferView];
				const gltf::Buffer& buffer = layout.buffers[bufferView.buffer];
				skins[i].inverseBindMatrices.resize(accessor.count);
				memcpy(&skins[i].inverseBindMatrices[0], &buffer.data[accessor.offset + bufferView.offset], accessor.count * sizeof(glm::mat4));
			}
		}
	}

	void Mesh_Data::loadAnimations(const gltf::Layout& layout) {
		const size_t animsCount = layout.animations.size();
		animations.resize(animsCount);

		for (size_t i = 0; i < animsCount; ++i) {
			const gltf::Animation& gltf_animation = layout.animations[i];
			Mesh_Animation& animation = animations[i];

			animation.name = gltf_animation.name;

			const size_t animSamplersCount = gltf_animation.samplers.size();
			animation.samplers.resize(animSamplersCount);

			for (size_t j = 0; j < animSamplersCount; ++j) {
				const gltf::AnimationSampler& gltf_animSampler = gltf_animation.samplers[j];
				Mesh_Animation::AnimationSampler& animSampler = animation.samplers[j];
				animSampler.interpolation = static_cast<Mesh_Animation::Interpolation>(gltf_animSampler.interpolation);

				// read keyframes input times
				{
					const gltf::Accessor& accessor = layout.accessors[gltf_animSampler.input];
					const gltf::BufferView& bufferView = layout.bufferViews[accessor.bufferView];
					const gltf::Buffer& buffer = layout.buffers[bufferView.buffer];
					const void* dataPtr = &buffer.data[accessor.offset + bufferView.offset];

					animSampler.inputs.resize(accessor.count);
					memcpy(&animSampler.inputs[0], dataPtr, accessor.count * sizeof(float));

					// adjust animation's start and end times
					for (auto input : animSampler.inputs) {
						if (input < animation.start) {
							animation.start = input;
						}

						if (input > animation.end) {
							animation.end = input;
						}
					}
				}

				animation.duration = animation.end - animation.start;

				// read sampler keyframe output translate/rotate/scale values
				{
					const gltf::Accessor& accessor = layout.accessors[gltf_animSampler.output];
					const gltf::BufferView& bufferView = layout.bufferViews[accessor.bufferView];
					const gltf::Buffer& buffer = layout.buffers[bufferView.buffer];

					const void* dataPtr = &buffer.data[accessor.offset + bufferView.offset];
					animSampler.outputs.resize(accessor.count);

					switch (accessor.type) {
						case gltf::AccessorType::VEC3: {
							struct vec3 { float x, y, z; };
							const vec3* buf = static_cast<const vec3*>(dataPtr);
							for (size_t index = 0; index < accessor.count; ++index) {
								animSampler.outputs[index] = glm::vec4(buf[index].x, buf[index].y, buf[index].z, 0.0f);
							}
							break;
						}
						case gltf::AccessorType::VEC4: {
							memcpy(&animSampler.outputs[0], dataPtr, accessor.count * sizeof(glm::vec4));
							break;
						}
						default:
							break;
					}
				}
			}

			// channels
			const size_t animCannelsCount = gltf_animation.channels.size();
			animation.channels.resize(animCannelsCount);
			animation.minTargetNodeId = 0xffff;
			animation.maxTargetNodeId = 0;
			for (size_t j = 0; j < animCannelsCount; ++j) {
				const gltf::AnimationChannel& gltf_channel = gltf_animation.channels[j];
				Mesh_Animation::AnimationChannel& channel = animation.channels[j];
				channel.path = static_cast<Mesh_Animation::AimationChannelPath>(gltf_channel.path);
				channel.sampler = gltf_channel.sampler;
				channel.target_node = gltf_channel.target_node;
				if (channel.target_node < animation.minTargetNodeId) { animation.minTargetNodeId = channel.target_node; }
				if (channel.target_node > animation.maxTargetNodeId) { animation.maxTargetNodeId = channel.target_node; }
			}
		}
	}

	void Mesh_Data::uploadGpuData(vulkan::VulkanBuffer*& vertices, vulkan::VulkanBuffer*& indices, const size_t vbOffset, const size_t ibOffset) {
		const uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(float);
		const uint32_t indexBufferSize = static_cast<uint32_t>(indexBuffer.size()) * sizeof(uint32_t);

		auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();

		stage_vertices = new vulkan::VulkanBuffer();
		stage_indices = new vulkan::VulkanBuffer();

		renderer->getDevice()->createBuffer(
			VK_SHARING_MODE_EXCLUSIVE,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stage_vertices,
			vertexBufferSize
		);

		renderer->getDevice()->createBuffer(
			VK_SHARING_MODE_EXCLUSIVE,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stage_indices,
			indexBufferSize
		);

		//
		if (vertices == nullptr) {
			vertices = new vulkan::VulkanBuffer();
			renderer->getDevice()->createBuffer(
				VK_SHARING_MODE_EXCLUSIVE,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				0,
				vertices,
				vertexBufferSize
			);
		}

		if (indices == nullptr) {
			indices = new vulkan::VulkanBuffer();
			renderer->getDevice()->createBuffer(
				VK_SHARING_MODE_EXCLUSIVE,
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				0,
				indices,
				indexBufferSize
			);
		}

		stage_vertices->upload(&vertexBuffer[0], vertexBufferSize, 0, false);
		stage_indices->upload(&indexBuffer[0], indexBufferSize, 0, false);

		destroyBuffers();

		indicesBuffer = indices;
		verticesBuffer = vertices;
		gpu_ibOffset = ibOffset;
		gpu_vbOffset = vbOffset;
	}

	void Mesh_Data::fillGpuData() {
		if (stage_vertices == nullptr || stage_indices == nullptr) return;

		auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();
		auto&& copyPool = renderer->getDevice()->getCommandPool(vulkan::GPUQueueFamily::F_GRAPHICS);
		auto&& cmdBuffer = renderer->getDevice()->createVulkanCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, copyPool);
		cmdBuffer.begin();

		VkBufferCopy regionV{ 0, gpu_vbOffset, stage_vertices->m_size };
		VkBufferCopy regionI{ 0, gpu_ibOffset, stage_indices->m_size };
		cmdBuffer.cmdCopyBuffer(*stage_vertices, *verticesBuffer, &regionV);
		cmdBuffer.cmdCopyBuffer(*stage_indices, *indicesBuffer, &regionI);

		cmdBuffer.flush(renderer->getMainQueue());

		stage_vertices->destroy();
		stage_indices->destroy();

		delete stage_vertices;
		delete stage_indices;
		stage_vertices = nullptr;
		stage_indices = nullptr;
	}

}