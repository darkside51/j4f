#pragma once

#include "../../Core/Math/mathematic.h"
#include "../../Core/Hierarchy.h"
#include "../../Core/Threads/ThreadPool.h"
#include "../../Core/Threads/ThreadPool2.h"
#include "../Render/RenderedEntity.h"

#include <memory>
#include <vector>

namespace engine {

	class MeshAnimationTree;

	struct Mesh_Node {
        inline static constexpr float epsilon = 1e-5f;
		uint16_t skinIndex = 0xffffu;
		vec3f translation = vec3f(0.0f, 0.0f, 0.0f);
		vec3f scale = vec3f(1.0f, 1.0f, 1.0f);
		quatf rotation = quatf(1.0f, 0.0f, 0.0f, 0.0f);
		
		bool dirtyLocalTransform = true;
		bool dirtyModelTransform = true;

		mat4f localMatrix = mat4f(1.0f);
		mat4f modelMatrix = mat4f(1.0f);

		inline void calculateLocalMatrix() {
			if (dirtyLocalTransform) {
				// TRS matrix : T * R * S
				memcpy(&localMatrix, &engine::emptyMatrix, sizeof(mat4f));
				translateMatrixTo(localMatrix, translation);
				quatToMatrix(rotation, localMatrix);
				scaleMatrix(localMatrix, scale);
				//
				dirtyLocalTransform = false;
				dirtyModelTransform = true;
			}
		}

		inline void calculateModelMatrix(const mat4f& parentModel) {
			modelMatrix = parentModel * localMatrix;
			// no set dirtyGlobalTransform = false here, for skin skin_matrices calculate
		}

		inline void setTranslation(const vec3f& t) {
			if (compare(t, translation, epsilon)) {
				translation = t;
				dirtyLocalTransform = true;
			}
		}

		inline void setTranslation(vec3f&& t) {
			if (compare(t, translation, epsilon)) {
				translation = std::move(t);
				dirtyLocalTransform = true;
			}
		}

		inline void setScale(const vec3f& s) {
			if (compare(s, scale, epsilon)) {
				scale = s;
				dirtyLocalTransform = true;
			}
		}

		inline void setScale(vec3f&& s) {
			if (compare(s, scale, epsilon)) {
				scale = std::move(s);
				dirtyLocalTransform = true;
			}
		}

		inline void setRotation(const quatf& r) {
			if (compare(r, rotation, epsilon)) {
				rotation = r;
				dirtyLocalTransform = true;
			}
		}

		inline void setRotation(quatf&& r) {
			if (compare(r, rotation, epsilon)) {
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
        friend void applyAnimationFrameToSkeleton(const CancellationToken& token, MeshSkeleton* skeleton, MeshAnimationTree* animTree, const uint8_t updateFrame);

	public:
		MeshSkeleton(Mesh_Data* mData, const uint8_t latency);
		~MeshSkeleton();

		Mesh_Node& getNode(const uint8_t updateFrame, const uint16_t nodeId) { return _nodes[updateFrame][nodeId]->value(); }
		[[nodiscard]] const Mesh_Node& getNode(const uint8_t updateFrame, const uint16_t nodeId) const { return _nodes[updateFrame][nodeId]->value(); }

        [[nodiscard]] inline uint8_t getUpdateFrame() const noexcept { return _updateFrameNum; }

		void updateAnimation(const float time, const Mesh_Animation* animation, float& currentAnimTime); // simple animation update
		void updateAnimation(const float time, MeshAnimationTree* animTree); // advanced animation update

        bool requestAnimUpdate() const noexcept { return _requestAnimUpdate; }
        void completeAnimUpdate() noexcept { _requestAnimUpdate = false; }
        void applyFrame(MeshAnimationTree* animTree); // animation another vision

		inline void checkAnimCalculation(const uint8_t frame) noexcept {
			if (_animCalculationResult[frame]) {
				if (const auto state = _animCalculationResult[frame]->state(); (state != TaskState::COMPLETE && state != TaskState::CANCELED)) {
					//if (_animCalculationResult[frame].valid() && _animCalculationResult[frame].wait_for(std::chrono::microseconds(0)) != std::future_status::ready) {
					_animCalculationResult[frame]->wait();
				}
			}
		}

        inline bool needSkipAnimCalculation(const uint8_t frame) noexcept {
            if (_animCalculationResult[frame]) {
                if (const auto state = _animCalculationResult[frame]->state(); (state != TaskState::COMPLETE && state != TaskState::CANCELED)) {
                   return true;
                }
            }
            return false;
        }

        [[nodiscard]] inline uint8_t getLatency() const noexcept { return _latency; }
        [[nodiscard]] inline bool dirtySkins() const noexcept { return _dirtySkins; }

		inline void setUseRootTransform(const bool use) noexcept { _useRootTransform = use; }
		[[nodeiscard]] inline bool getUseRootTransform() const noexcept { return _useRootTransform; }

	private:
		void loadNode(const Mesh_Data* mData, const uint16_t nodeId, HierarchyUnique<Mesh_Node>* parent, const uint8_t h);

		void updateSkins(const uint8_t updateFrame);
		void updateTransforms(const uint8_t updateFrame);

        inline void setUpdatedFrameNum(const uint8_t frame) noexcept {
            _updatedFrameNum.store(frame, std::memory_order_relaxed);
        }
        inline uint8_t getUpdatedFrameNum() const noexcept {
            return _updatedFrameNum.load(std::memory_order_relaxed);
        }

		struct HierarchyMatrixUpdater {
			inline static bool _(HierarchyUnique<Mesh_Node>* node) {
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
						memcpy(&mNode.modelMatrix, &mNode.localMatrix, sizeof(mat4f));
					}
				}

				return true;
			}
		};

		inline static bool updateHierarchyMatrix(HierarchyUnique<Mesh_Node>* node) {
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
					memcpy(&mNode.modelMatrix, &mNode.localMatrix, sizeof(mat4f));
				}
			}

			return true;
		}  

		const std::vector<Mesh_Skin>& _skins;
		std::vector<std::vector<std::unique_ptr<HierarchyUnique<Mesh_Node>>>> _hierarchyes;
		std::vector<std::vector<HierarchyUnique<Mesh_Node>*>> _nodes;
		std::vector<std::vector<std::vector<mat4f>>> _skinsMatrices;
		std::vector<linked_ptr<Task2<void>>> _animCalculationResult;

		uint8_t _latency = 1u;
		uint8_t _updateFrameNum = 0u;
		bool _dirtySkins = true;
        bool _requestAnimUpdate = false;
		bool _useRootTransform = true;

        std::atomic<uint8_t> _updatedFrameNum = {0u};
	};

	class Mesh : public RenderedEntity {
	public:
		~Mesh() override;

		void createWithData(Mesh_Data* mData, const uint16_t semantic_mask, const uint8_t latency);

		void createRenderData();

		inline uint16_t getNodesCount() const { return _skeleton->_nodes[0].size(); }

		Mesh_Node& getNode(const uint8_t updateFrame, const uint16_t nodeId) { return _skeleton->_nodes[updateFrame][nodeId]->value(); }
		const Mesh_Node& getNode(const uint8_t updateFrame, const uint16_t nodeId) const { return _skeleton->_nodes[updateFrame][nodeId]->value(); }

		inline Mesh_Data* getMeshData() noexcept { return _meshData; }
		inline const Mesh_Data* getMeshData() const noexcept { return _meshData; }

		inline const vec3f& getMinCorner() const noexcept { return _minCorner; }
		inline const vec3f& getMaxCorner() const noexcept { return _maxCorner; }

		inline std::shared_ptr<MeshSkeleton>& getSkeleton() noexcept { return _skeleton; }
		inline const std::shared_ptr<MeshSkeleton>& getSkeleton() const noexcept { return _skeleton; }

		inline void setSkeleton(const std::shared_ptr<MeshSkeleton>& s) noexcept { _skeleton = s; }

		void draw(const mat4f& cameraMatrix, const mat4f& worldMatrix, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame);
		void drawBoundingBox(const mat4f& cameraMatrix, const mat4f& worldMatrix, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame);

		void updateRenderData(RenderDescriptor & renderDescriptor, const mat4f& worldMatrix, const bool worldMatrixChanged);
		inline void updateModelMatrixChanged(const bool worldMatrixChanged) noexcept { _modelMatrixChanged |= worldMatrixChanged; }

		inline void setCameraMatrix(const mat4f& cameraMatrix, const bool copy = false) {
			_renderDescriptor.setRawDataForLayout(_fixedGpuLayouts[0].first, &const_cast<mat4f&>(cameraMatrix), copy, sizeof(mat4f));
		}

	private:
        VertexAttributes getVertexInputAttributes() const;
		uint32_t sizeOfVertex() const;

		Mesh_Data* _meshData = nullptr;

		uint16_t _semanticMask = 0u;

		std::shared_ptr<MeshSkeleton> _skeleton;
		bool _modelMatrixChanged = true;

		vec3f _minCorner = vec3f(0.0f);
		vec3f _maxCorner = vec3f(0.0f);
	};
}