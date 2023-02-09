#pragma once

#include "Node.h"
#include "NodeGraphicsLink.h"
#include "../Render/RenderList.h"

namespace engine {

	class Frustum;

	class FrustumVisibleChecker final {
	public:
		FrustumVisibleChecker() = default;
		FrustumVisibleChecker(const Frustum* f) : _frustum(f) {}
		~FrustumVisibleChecker() = default;

		inline bool operator()(const BoundingVolume* volume, const glm::mat4& wtr) const {
			return volume->checkVisible<Frustum>(_frustum, wtr);
		}

	private:
		const Frustum* _frustum = nullptr;
	};

	template<typename V>
	struct RenderListEmplacer final {
		inline static bool _(H_Node* node, RenderList& list, const bool dirtyVisible, const uint8_t visibleId, V&& visibleChecker) {
			if (NodeUpdater::_<V>(node, dirtyVisible, visibleId, std::forward<V>(visibleChecker))) {
				if (NodeRenderObject* renderObject = node->value().getRenderObject()) {
					renderObject->setNeedUpdate(true);
					list.addDescriptor(renderObject->getRenderDescriptor());
				}
				return true;
			}

			return false;
		}
	};

	template<typename V>
	struct NodeAndRenderObjectUpdater final {
		inline static bool _(H_Node* node, const bool dirtyVisible, const uint8_t visibleId, V&& visibleChecker) {
			const bool visible = NodeUpdater::_<V>(node, dirtyVisible, visibleId, std::forward<V>(visibleChecker));

			if (NodeRenderObject* renderObject = node->value().getRenderObject()) {
				renderObject->setNeedUpdate(visible);
				renderObject->getRenderDescriptor()->visible = visible;
			}

			return true;
		}
	};

	// reload list by nodes hierarchy
	template<typename V = EmptyVisibleChecker>
	inline void reloadRenderList(RenderList& list, H_Node* node, const bool dirtyVisible, const uint8_t visibleId, V&& visibleChecker = V()) {
		using list_emplacer_type = RenderListEmplacer<V>;
		list.clear();
		node->execute_with<list_emplacer_type>(list, dirtyVisible, visibleId, std::forward<V>(visibleChecker));
		list.sort();
	}

	// reload list by nodes hierarchy array
	template<typename V = EmptyVisibleChecker>
	inline void reloadRenderList(RenderList& list, H_Node** node, size_t count, const bool dirtyVisible, const uint8_t visibleId, V&& visibleChecker = V()) {
		using list_emplacer_type = RenderListEmplacer<V>;
		list.clear();
		for (size_t i = 0; i < count; ++i) {
			node[i]->execute_with<list_emplacer_type>(list, dirtyVisible, visibleId, std::forward<V>(visibleChecker));
		}
		list.sort();
	}

	// reload list by some parts of nodes hierarchy
	inline void startReloadRenderList(RenderList& list) {
		list.clear();
	}

	template<typename V = EmptyVisibleChecker>
	inline void addNodeHierarchyToRenderList(RenderList& list, H_Node* node, const bool dirtyVisible, const uint8_t visibleId, V&& visibleChecker = V()) {
		using list_emplacer_type = RenderListEmplacer<V>;
		node->execute_with<list_emplacer_type>(list, dirtyVisible, visibleId, std::forward<V>(visibleChecker));
	}

	inline void finishReloadRenderList(RenderList& list) {
		list.sort();
	}
	// reload list by some parts of nodes hierarchy

	template<typename V = EmptyVisibleChecker>
	inline void updateNodeRenderObjects(H_Node* node, const bool dirtyVisible, const uint8_t visibleId, V&& visibleChecker = V()) {
		using objects_updater_type = NodeAndRenderObjectUpdater<V>;
		node->execute_with<objects_updater_type>(dirtyVisible, visibleId, std::forward<V>(visibleChecker));
	}

	template<typename V = EmptyVisibleChecker>
	inline void updateNodeRenderObjects(H_Node** node, size_t count, const bool dirtyVisible, const uint8_t visibleId, V&& visibleChecker = V()) {
		using objects_updater_type = NodeAndRenderObjectUpdater<V>;
		for (size_t i = 0; i < count; ++i) {
			node[i]->execute_with<objects_updater_type>(dirtyVisible, visibleId, std::forward<V>(visibleChecker));
		}
	}
}