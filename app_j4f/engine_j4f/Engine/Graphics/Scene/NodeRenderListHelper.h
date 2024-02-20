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

		inline bool operator()(const BoundingVolume* volume, const mat4f& wtr) const {
			return volume->checkVisible<Frustum>(_frustum, wtr);
		}

	private:
		const Frustum* _frustum = nullptr;
	};

	template<typename V, typename T = Empty>
	struct RenderListEmplacer final {
		inline static bool _(NodeHR* node, RenderList& list, const bool dirtyVisible, const uint8_t visibleId, V&& visibleChecker,
                             bool needResetChanged, const T& callback = {}) {
			if (NodeUpdater::_<V>(node, dirtyVisible, visibleId, std::forward<V>(visibleChecker), needResetChanged)) {
				if (NodeRenderer* renderObject = node->value().getRenderer()) {
					renderObject->setNeedUpdate(true);
                    if constexpr (!std::is_same_v<T, Empty>) {
                        callback(renderObject->getRenderEntity());
                    }
					list.addEntity(renderObject->getRenderEntity());
				}
				return true;
			}

			return false;
		}
	};

	template<typename V>
	struct NodeAndRenderObjectUpdater final {
		inline static bool _(NodeHR* node, const bool dirtyVisible, const uint8_t visibleId, V&& visibleChecker) {
			const bool visible = NodeUpdater::_<V>(node, dirtyVisible, visibleId, std::forward<V>(visibleChecker));

			if (NodeRenderer* renderObject = node->value().getRenderer()) {
				renderObject->setNeedUpdate(visible);
				renderObject->getRenderDescriptor().visible = visible;
			}

			return true;
		}
	};

	// reload list by nodes hierarchy
	template<typename V = EmptyVisibleChecker, typename T = Empty>
	inline void reloadRenderList(RenderList& list, NodeHR* node, const bool dirtyVisible, const uint8_t visibleId,
                                 V&& visibleChecker = V(), bool needResetChanged = true, T&& callback = {}) {
		using list_emplacer_type = RenderListEmplacer<V, T>;
		list.clear();
		node->execute_with<list_emplacer_type>(list, dirtyVisible, visibleId, std::forward<V>(visibleChecker),
		        needResetChanged, callback);
		list.sort();
	}

	// reload list by nodes hierarchy array
	template<typename V = EmptyVisibleChecker, typename T = Empty>
	inline void reloadRenderList(RenderList& list, NodeHR** node, size_t count, const bool dirtyVisible,
                                 const uint8_t visibleId, V&& visibleChecker = V(), bool needResetChanged = true,
                                 T&& callback = {}) {
		using list_emplacer_type = RenderListEmplacer<V, T>;
		list.clear();
		for (size_t i = 0u; i < count; ++i) {
			node[i]->execute_with<list_emplacer_type>(list, dirtyVisible, visibleId,
                                                      std::forward<V>(visibleChecker), needResetChanged, callback);
		}
		list.sort();
	}

    // reload list by nodes hierarchy array
    template<typename V = EmptyVisibleChecker, typename T = Empty>
    inline void reloadRenderList(RenderList& list, engine::ref_ptr<NodeHR>* node, size_t count, const bool dirtyVisible,
                                 const uint8_t visibleId, V&& visibleChecker = {}, bool needResetChanged = true,
                                 T&& callback = {}, bool withChildren = false) {
        using list_emplacer_type = RenderListEmplacer<V, T>;
        list.clear();
        for (size_t i = 0u; i < count; ++i) {
            if (withChildren) {
                node[i]->execute_with<list_emplacer_type>(list, dirtyVisible, visibleId,
                                                          std::forward<V>(visibleChecker), needResetChanged, callback);
            } else {
                list_emplacer_type::_(node[i].get(), list, dirtyVisible, visibleId,
                                      std::forward<V>(visibleChecker), needResetChanged, callback);
            }
        }
        list.sort();
    }

	// reload list by some parts of nodes hierarchy
	inline void startReloadRenderList(RenderList& list) {
		list.clear();
	}

	template<typename V = EmptyVisibleChecker>
	inline void addNodeHierarchyToRenderList(RenderList& list, NodeHR* node, const bool dirtyVisible, const uint8_t visibleId, V&& visibleChecker = V()) {
		using list_emplacer_type = RenderListEmplacer<V>;
		node->execute_with<list_emplacer_type>(list, dirtyVisible, visibleId, std::forward<V>(visibleChecker));
	}

	inline void finishReloadRenderList(RenderList& list) {
		list.sort();
	}
	// reload list by some parts of nodes hierarchy

	template<typename V = EmptyVisibleChecker>
	inline void updateNodeRenderers(NodeHR* node, const bool dirtyVisible, const uint8_t visibleId, V&& visibleChecker = V()) {
		using objects_updater_type = NodeAndRenderObjectUpdater<V>;
		node->execute_with<objects_updater_type>(dirtyVisible, visibleId, std::forward<V>(visibleChecker));
	}

	template<typename V = EmptyVisibleChecker>
	inline void updateNodeRenderers(NodeHR** node, size_t count, const bool dirtyVisible, const uint8_t visibleId, V&& visibleChecker = V()) {
		using objects_updater_type = NodeAndRenderObjectUpdater<V>;
		for (size_t i = 0; i < count; ++i) {
			node[i]->execute_with<objects_updater_type>(dirtyVisible, visibleId, std::forward<V>(visibleChecker));
		}
	}
}