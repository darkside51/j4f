#pragma once

#include "../../Core/Math/math.h"
#include "../../Core/Hierarchy.h"
#include "../../Core/Threads/ThreadPool.h"
#include "../Vulkan/vkRenderData.h"

#include "../Render/RenderDescriptor.h"

#include <string>
#include <vector>
#include <cmath>
#include <memory>

namespace engine {

	class MeshAnimationTree;

	struct Mesh_Node {
		uint16_t skinIndex = 0xffff;
		glm::vec3 translation;
		glm::vec3 scale;
		glm::quat rotation;
		
		bool dirtyLocalTransform = true;
		bool dirtyModelTransform = true;

		glm::mat4 localMatrix;
		glm::mat4 modelMatrix;

		inline void calculateLocalMatrix() {
			if (dirtyLocalTransform) {
				// TRS matrix : T * R * S
				memcpy(&localMatrix, &engine::emptyMatrix, sizeof(glm::mat4));
				translateMatrixTo(localMatrix, translation);
				quatToMatrix(rotation, localMatrix);
				scaleMatrix(localMatrix, scale);
				//
				dirtyLocalTransform = false;
				dirtyModelTransform = true;
			}
		}

		inline void calculateModelMatrix(const glm::mat4& parentModel) {
			modelMatrix = parentModel * localMatrix;
			// no set dirtyGlobalTransform = false here, for skin skin_matrices calculate
		}

		inline void setTranslation(const glm::vec3& t) {
			if (compare(t, translation, 1e-5f)) {
				translation = t;
				dirtyLocalTransform = true;
			}
		}

		inline void setTranslation(glm::vec3&& t) {
			if (compare(t, translation, 1e-5f)) {
				translation = std::move(t);
				dirtyLocalTransform = true;
			}
		}

		inline void setScale(const glm::vec3& s) {
			if (compare(s, scale, 1e-5f)) {
				scale = s;
				dirtyLocalTransform = true;
			}
		}

		inline void setScale(glm::vec3&& s) {
			if (compare(s, scale, 1e-5f)) {
				scale = std::move(s);
				dirtyLocalTransform = true;
			}
		}

		inline void setRotation(const glm::quat& r) {
			if (compare(r, rotation, 1e-5f)) {
				rotation = r;
				dirtyLocalTransform = true;
			}
		}

		inline void setRotation(glm::quat&& r) {
			if (compare(r, rotation, 1e-5f)) {
				rotation = std::move(r);
				dirtyLocalTransform = true;
			}
		}
	};

	struct Mesh_Data;
	struct Mesh_Animation;
	struct Mesh_Skin;

	class Skeleton {
		friend class Mesh;
		friend void updateSkeletonAnimation(const CancellationToken& token, Skeleton* skeleton, const float time, const Mesh_Animation* animation, const uint8_t updateFrame);
		friend void updateSkeletonAnimationTree(const CancellationToken& token, Skeleton* skeleton, MeshAnimationTree* animTree, const uint8_t updateFrame);

	public:
		Skeleton(Mesh_Data* mData, const uint8_t latency);
		~Skeleton();

		void loadNode(const Mesh_Data* mData, const uint16_t nodeId, HierarchyRaw<Mesh_Node>* parent, const uint8_t h);

		Mesh_Node& getNode(const uint8_t updateFrame, const uint16_t nodeId) { return _nodes[updateFrame][nodeId]->value(); }
		const Mesh_Node& getNode(const uint8_t updateFrame, const uint16_t nodeId) const { return _nodes[updateFrame][nodeId]->value(); }

		inline uint8_t getUpdateFrame() const { return _updateFrameNum; }

		void updateAnimation(const float time, const Mesh_Animation* animation, float& currentAnimTime); // simple animation update
		void updateAnimation(const float time, MeshAnimationTree* animTree); // advanced animation update

		inline void checkAnimCalculation(const uint8_t frame) {
			if (_animCalculationResult[frame].valid() && _animCalculationResult[frame].wait_for(std::chrono::microseconds(0)) != std::future_status::ready) {
				_animCalculationResult[frame].wait();
			}
		}

		inline uint8_t getLatency() const { return _latency; }

	private:
		void updateSkins(const uint8_t updateFrame);
		void updateTransforms(const uint8_t updateFrame);

		inline static bool updateHierarhyMatrix(HierarchyRaw<Mesh_Node>* node) {
			Mesh_Node& mNode = node->value();
			mNode.dirtyModelTransform = false;
			mNode.calculateLocalMatrix();

			auto&& parent = node->getParent();
			if (parent) {
				mNode.dirtyModelTransform |= parent->value().dirtyModelTransform;
			}

			if (mNode.dirtyModelTransform) {
				if (parent) {
					mNode.calculateModelMatrix(parent->value().modelMatrix);
				} else {
					memcpy(&mNode.modelMatrix, &mNode.localMatrix, sizeof(glm::mat4));
				}
			}

			return true;
		}  

		const std::vector<Mesh_Skin>& _skins;
		std::vector<std::vector<HierarchyRaw<Mesh_Node>*>> _hierarchyes;
		std::vector<std::vector<HierarchyRaw<Mesh_Node>*>> _nodes;
		std::vector<std::vector<std::vector<glm::mat4>>> _skinsMatrices;
		std::vector<TaskResult<void>> _animCalculationResult;

		uint8_t _latency;
		uint8_t _updateFrameNum = 0;
	};

	class Mesh {
	public:
		~Mesh();

		void loadNode(const uint16_t nodeId, HierarchyRaw<Mesh_Node>* parent, const uint8_t h);
		void createWithData(Mesh_Data* mData, const uint16_t semantic_mask, const uint8_t latency);

		void createRenderData();

		std::vector<VkVertexInputAttributeDescription> getVertexInputAttributes() const;
		uint32_t sizeOfVertex() const;

		void setPipeline(vulkan::VulkanPipeline* p);
		void setProgram(vulkan::VulkanGpuProgram* program, VkRenderPass renderPass = nullptr);

		inline uint16_t getNodesCount() const { return _skeleton->_nodes[0].size(); }

		Mesh_Node& getNode(const uint8_t updateFrame, const uint16_t nodeId) { return _skeleton->_nodes[updateFrame][nodeId]->value(); }
		const Mesh_Node& getNode(const uint8_t updateFrame, const uint16_t nodeId) const { return _skeleton->_nodes[updateFrame][nodeId]->value(); }

		inline Mesh_Data* getMeshData() { return _meshData; }
		inline const Mesh_Data const* getMeshData() const { return _meshData; }

		inline const glm::vec3& getMinCorner() const { return _minCorner; }
		inline const glm::vec3& getMaxCorner() const { return _maxCorner; }

		inline std::shared_ptr<Skeleton>& getSkeleton() { return _skeleton; }
		inline const std::shared_ptr<Skeleton>& getSkeleton() const { return _skeleton; }

		inline void setSkeleton(const std::shared_ptr<Skeleton>& s) {
			if (_skeleton && _skeleton != s) {
				_skeleton = s;
			}
		}

		template <typename T>
		inline void setParamByName(const std::string& name, T* value, bool copyData, const uint32_t count = 1) {
			for (uint32_t i = 0; i < _renderDescriptor.renderDataCount; ++i) {
				_renderDescriptor.renderData[i]->setParamByName(name, value, copyData, count);
			}
		}

		template <typename T>
		inline void setParamForLayout(const vulkan::GPUParamLayoutInfo* info, T* value, const bool copyData, const uint32_t count = 1) {
			for (uint32_t i = 0; i < _renderDescriptor.renderDataCount; ++i) {
				_renderDescriptor.renderData[i]->setParamForLayout(info, value, copyData, count);
			}
		}

		void draw(const glm::mat4& cameraMatrix, const glm::mat4& worldMatrix, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame);
		void drawBoundingBox(const glm::mat4& cameraMatrix, const glm::mat4& worldMatrix, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame);

		void updateRenderData(const glm::mat4& worldMatrix);

		inline void setCameraMatrix(const glm::mat4& cameraMatrix, const bool copy = false) {
			_renderDescriptor.setRawDataForLayout(_camera_matrix, &const_cast<glm::mat4&>(cameraMatrix), copy, sizeof(glm::mat4));
		}

		inline void render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const glm::mat4* cameraMatrix) {
			_renderDescriptor.render(commandBuffer, currentFrame, cameraMatrix);
		}

		inline vulkan::RenderData* getRenderDataAt(const uint16_t n) const {
			if (_renderDescriptor.renderDataCount <= n) return nullptr;
			return _renderDescriptor.renderData[n];
		}

		inline vulkan::VulkanRenderState& renderState() { return _renderState; }
		inline const vulkan::VulkanRenderState& renderState() const { return _renderState; }

		void onPipelineAttributesChanged();

		inline const RenderDescriptor& getRenderDescriptor() const { return _renderDescriptor; }
		inline RenderDescriptor& getRenderDescriptor() { return _renderDescriptor; }

	private:
		Mesh_Data* _meshData = nullptr;
		RenderDescriptor _renderDescriptor;

		uint16_t _semanticMask = 0;

		std::shared_ptr<Skeleton> _skeleton;

		glm::vec3 _minCorner = glm::vec3(0.0f);
		glm::vec3 _maxCorner = glm::vec3(0.0f);

		vulkan::VulkanRenderState _renderState;

		const vulkan::GPUParamLayoutInfo* _camera_matrix = nullptr;
		const vulkan::GPUParamLayoutInfo* _model_matrix = nullptr;
		const vulkan::GPUParamLayoutInfo* _skin_matrices = nullptr;
		const vulkan::GPUParamLayoutInfo* _use_skin = nullptr;
	};
}