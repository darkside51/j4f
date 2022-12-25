#pragma once

#include "../../Core/Math/math.h"
#include "../../Core/Hierarchy.h"
#include "../../Core/Threads/ThreadPool.h"
#include "../../Core/Threads/ThreadPool2.h"
#include "../Render/RenderedEntity.h"

#include <memory>
#include <vector>

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

	class MeshSkeleton {
		friend class Mesh;
		friend void updateSkeletonAnimation(const CancellationToken& token, MeshSkeleton* skeleton, const float time, const Mesh_Animation* animation, const uint8_t updateFrame);
		friend void updateSkeletonAnimationTree(const CancellationToken& token, MeshSkeleton* skeleton, MeshAnimationTree* animTree, const uint8_t updateFrame);

	public:
		MeshSkeleton(Mesh_Data* mData, const uint8_t latency);
		~MeshSkeleton();

		void loadNode(const Mesh_Data* mData, const uint16_t nodeId, HierarchyRaw<Mesh_Node>* parent, const uint8_t h);

		Mesh_Node& getNode(const uint8_t updateFrame, const uint16_t nodeId) { return _nodes[updateFrame][nodeId]->value(); }
		const Mesh_Node& getNode(const uint8_t updateFrame, const uint16_t nodeId) const { return _nodes[updateFrame][nodeId]->value(); }

		inline uint8_t getUpdateFrame() const noexcept { return _updateFrameNum; }

		void updateAnimation(const float time, const Mesh_Animation* animation, float& currentAnimTime); // simple animation update
		void updateAnimation(const float time, MeshAnimationTree* animTree); // advanced animation update

		inline void checkAnimCalculation(const uint8_t frame) {
			if (_animCalculationResult[frame]) {
				if (const auto state = _animCalculationResult[frame]->state(); (state != TaskState::COMPLETE && state != TaskState::CANCELED)) {
					//if (_animCalculationResult[frame].valid() && _animCalculationResult[frame].wait_for(std::chrono::microseconds(0)) != std::future_status::ready) {
					_animCalculationResult[frame]->wait();
				}
			}
		}

		inline uint8_t getLatency() const noexcept { return _latency; }
		inline bool dirtySkins() const noexcept { return _dirtySkins; }

	private:
		void updateSkins(const uint8_t updateFrame);
		void updateTransforms(const uint8_t updateFrame);

		struct HierarchyMatrixUpdater {
			inline static bool _(HierarchyRaw<Mesh_Node>* node) {
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
		};

		inline static bool updateHierarchyMatrix(HierarchyRaw<Mesh_Node>* node) {
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
		//std::vector<TaskResult<void>> _animCalculationResult;
		std::vector<linked_ptr<Task2<void>>> _animCalculationResult;

		uint8_t _latency;
		uint8_t _updateFrameNum = 0;
		bool _dirtySkins = true;
	};

	class Mesh : public RenderedEntity {
	public:
		~Mesh();

		void createWithData(Mesh_Data* mData, const uint16_t semantic_mask, const uint8_t latency);

		void createRenderData();

		inline uint16_t getNodesCount() const { return _skeleton->_nodes[0].size(); }

		Mesh_Node& getNode(const uint8_t updateFrame, const uint16_t nodeId) { return _skeleton->_nodes[updateFrame][nodeId]->value(); }
		const Mesh_Node& getNode(const uint8_t updateFrame, const uint16_t nodeId) const { return _skeleton->_nodes[updateFrame][nodeId]->value(); }

		inline Mesh_Data* getMeshData() noexcept { return _meshData; }
		inline const Mesh_Data* getMeshData() const noexcept { return _meshData; }

		inline const glm::vec3& getMinCorner() const noexcept { return _minCorner; }
		inline const glm::vec3& getMaxCorner() const noexcept { return _maxCorner; }

		inline std::shared_ptr<MeshSkeleton>& getSkeleton() noexcept { return _skeleton; }
		inline const std::shared_ptr<MeshSkeleton>& getSkeleton() const noexcept { return _skeleton; }

		inline void setSkeleton(const std::shared_ptr<MeshSkeleton>& s) noexcept { _skeleton = s; }

		void draw(const glm::mat4& cameraMatrix, const glm::mat4& worldMatrix, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame);
		void drawBoundingBox(const glm::mat4& cameraMatrix, const glm::mat4& worldMatrix, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame);

		void updateRenderData(const glm::mat4& worldMatrix, const bool worldMatrixChanged);
		inline void updateModelMatrixChanged(const bool worldMatrixChanged) noexcept { _modelMatrixChanged |= worldMatrixChanged; }

		inline void setCameraMatrix(const glm::mat4& cameraMatrix, const bool copy = false) {
			_renderDescriptor.setRawDataForLayout(_fixedGpuLayouts[0].first, &const_cast<glm::mat4&>(cameraMatrix), copy, sizeof(glm::mat4));
		}

	private:
		std::vector<VkVertexInputAttributeDescription> getVertexInputAttributes() const;
		uint32_t sizeOfVertex() const;

		Mesh_Data* _meshData = nullptr;

		uint16_t _semanticMask = 0;

		std::shared_ptr<MeshSkeleton> _skeleton;
		bool _modelMatrixChanged = true;

		glm::vec3 _minCorner = glm::vec3(0.0f);
		glm::vec3 _maxCorner = glm::vec3(0.0f);
	};
}