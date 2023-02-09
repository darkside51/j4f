#pragma once

#include "../../Core/Hierarchy.h"
#include "../../Core/Math/mathematic.h"
#include "../../Core/BitMask.h"

#include "BoundingVolume.h"

namespace engine {
	
	class NodeRenderObject;

	class Node final {
		friend struct NodeUpdater;
	public:
		~Node();

		inline void calculateModelMatrix(const glm::mat4& parentModel) {
			_model = parentModel * _local;
			_dirtyModel = false;
			_modelChanged = true;
		}

		inline void setLocalMatrix(const glm::mat4& m) {
			memcpy(&_local, &m, sizeof(glm::mat4));
			_dirtyModel = true;
		}

		inline const glm::mat4& localMatrix() const { return _local; }
		inline const glm::mat4& model() const { return _model; }
		inline bool modelChanged() const { return _modelChanged; }

		inline NodeRenderObject* getRenderObject() { return _graphics; }
		inline const NodeRenderObject* getRenderObject() const { return _graphics; }

		void setRenderObject(const NodeRenderObject* r);

		inline const BoundingVolume* getBoundingVolume() const { return _boundingVolume; }
		inline void setBoundingVolume(const BoundingVolume* v) {
			if (_boundingVolume) {
				delete _boundingVolume;
			}
			_boundingVolume = v;
		}

		inline BitMask64& visible() { return _visibleMask; }
		inline const BitMask64& visible() const { return _visibleMask; }

		inline void setVisible(const uint8_t visibleId, const bool value) {
			_visibleMask.setBit(visibleId, value);
		}

		inline bool isVisible(const uint8_t visibleId) const {
			return _visibleMask.checkBit(visibleId);
		}

	private:
		bool _dirtyModel = false;
		bool _modelChanged = false;
		glm::mat4 _local = glm::mat4(1.0f);
		glm::mat4 _model = glm::mat4(1.0f);
		NodeRenderObject* _graphics = nullptr;
		const BoundingVolume* _boundingVolume = nullptr;
		BitMask64 _visibleMask; // ����� ��������� (��������������, ��� ������ ����� ���� ������� ��� ��� � ���������� ����������, ��� ���������� ��������� � ������� ����� ������������ BitMask64)
	};

	using H_Node = HierarchyRaw<Node>;
	using Hs_Node = HierarchyShared<Node>;

	class EmptyVisibleChecker {};

	struct NodeUpdater final {
		template<typename V>
		inline static bool _(H_Node* node, const bool dirtyVisible, const uint8_t visibleId, V&& visibleChecker) {
			Node& mNode = node->value();
			mNode._modelChanged = false;

			const auto&& parent = node->getParent();
			if (parent) {
				mNode._dirtyModel |= parent->value()._modelChanged;
			}

			if (mNode._dirtyModel) {
				if (parent) {
					mNode.calculateModelMatrix(parent->value()._model);
				} else {
					memcpy(&mNode._model, &mNode._local, sizeof(glm::mat4));
					mNode._dirtyModel = false;
					mNode._modelChanged = true;
				}
			}

			if constexpr (std::is_same_v<V, EmptyVisibleChecker>) {
				return mNode._boundingVolume ? mNode.isVisible(visibleId) : true;
			} else {
				if (dirtyVisible || mNode._modelChanged) {
					if (const BoundingVolume* volume = mNode._boundingVolume) {
						const bool visible = visibleChecker(volume, mNode._model);
						mNode.setVisible(visibleId, visible);
						return visible;
					} else {
						mNode.setVisible(visibleId, true);
						return true;
					}
				} else {
					return mNode._boundingVolume ? mNode.isVisible(visibleId) : true;
				}
			}
		}
	};

	// render bounding volumes for hierarchy
	inline void renderNodesBounds(H_Node* node, const glm::mat4& cameraMatrix, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const uint8_t visibleId = 0) {
		node->execute([](H_Node* node, const glm::mat4& cameraMatrix, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const uint8_t visibleId) -> bool {
			Node& mNode = node->value();
			if (auto&& volume = mNode.getBoundingVolume()) {
				if (mNode.visible().checkBit(visibleId)) {
					volume->render(cameraMatrix, mNode.model(), commandBuffer, currentFrame);
					return true;
				}
				return false;
			} else {
				return true;
			}
		},
		cameraMatrix, commandBuffer, currentFrame, visibleId);
	}
}