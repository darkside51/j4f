#pragma once

#include "../../Core/Common.h"
#include "../../Core/Hierarchy.h"
#include "../../Core/Math/mathematic.h"
#include "../../Core/BitMask.h"

#include "BoundingVolume.h"

#include <memory>

namespace engine {
	
	class NodeRenderer;

	class Node final {
		friend struct NodeUpdater;
	public:
		explicit Node(NodeRenderer* graphics);
		Node() = default;
		~Node();

		inline void calculateModelMatrix(const mat4f& parentModel) noexcept {
			_model = parentModel * _local;
			_dirtyModel = false;
			_modelChanged = true;
		}

		inline void setLocalMatrix(const mat4f& m) noexcept {
			memcpy(&_local, &m, sizeof(mat4f));
			_dirtyModel = true;
		}

		inline const mat4f& localMatrix() const noexcept { return _local; }
		inline const mat4f& model() const noexcept { return _model; }
		inline bool modelChanged() const noexcept { return _modelChanged; }

		inline NodeRenderer* getRenderer() noexcept { return _renderer; }
		inline const NodeRenderer* getRenderer() const noexcept { return _renderer; }

		template <typename T>
		inline T* getRenderer() noexcept {
			static_assert(static_inheritance::isInherit<T, NodeRenderer>());
			return static_cast<T*>(_renderer);
		}
		template <typename T>
		inline const T* getRenderer() const noexcept {
			static_assert(static_inheritance::isInherit<T, NodeRenderer>());
			return static_cast<T*>(_renderer);
		}

		void setRenderer(const NodeRenderer* r);

		inline const BoundingVolume* getBoundingVolume() const noexcept { return _boundingVolume.get(); }
		inline void setBoundingVolume(std::unique_ptr<const BoundingVolume>&& v) {
			_boundingVolume = std::move(v);
		}

		inline BitMask64& visible() noexcept { return _visibleMask; }
		inline const BitMask64& visible() const noexcept { return _visibleMask; }

		inline void setVisible(const uint8_t visibleId, const bool value) {
			_visibleMask.setBit(visibleId, value);
		}

		inline bool isVisible(const uint8_t visibleId) const noexcept {
			return _visibleMask.checkBit(visibleId);
		}

	private:
		bool _dirtyModel = false;
		bool _modelChanged = false;
		mat4f _local = mat4f(1.0f);
		mat4f _model = mat4f(1.0f);
		NodeRenderer* _renderer = nullptr;
		std::unique_ptr<const BoundingVolume> _boundingVolume = nullptr;
		BitMask64 _visibleMask; // ����� ��������� (��������������, ��� ������ ����� ���� ������� ��� ��� � ���������� ����������, ��� ���������� ��������� � ������� ����� ������������ BitMask64)
	};

	using NodeHR = HierarchyRaw<Node>;
	using NodeHS = HierarchyShared<Node>;
	using NodeHU = HierarchyUnique<Node>;

	class EmptyVisibleChecker {};

	struct NodeUpdater final {
		template<typename V>
		inline static bool _(NodeHR* node, const bool dirtyVisible, const uint8_t visibleId, V&& visibleChecker, bool needResetChanged) {
			Node& mNode = node->value();
            if (needResetChanged) {
                mNode._modelChanged = false;
            }

			const auto&& parent = node->getParent();
			if (parent) {
				mNode._dirtyModel |= parent->value()._modelChanged;
			}

			if (mNode._dirtyModel) {
				if (parent) {
					mNode.calculateModelMatrix(parent->value()._model);
				} else {
					memcpy(&mNode._model, &mNode._local, sizeof(mat4f));
					mNode._dirtyModel = false;
					mNode._modelChanged = true;
				}
			}

			if constexpr (std::is_same_v<V, EmptyVisibleChecker>) {
				return mNode._boundingVolume ? mNode.isVisible(visibleId) : true;
			} else {
				if (dirtyVisible || mNode._modelChanged) {
					if (auto&& volume = mNode._boundingVolume) {
						const bool visible = visibleChecker(volume.get(), mNode._model);
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
	inline void renderNodesBounds(NodeHR* node, const mat4f& cameraMatrix, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const uint8_t visibleId = 0) {
		node->execute([](NodeHR* node, const mat4f& cameraMatrix, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const uint8_t visibleId) -> bool {
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