#pragma once

#include "Node.h"
#include "../Render/RenderList.h"

namespace engine {

	class FrustumVisibleChecker {
	public:
		FrustumVisibleChecker() = default;
		FrustumVisibleChecker(const Frustum* f) : _frustum(f) {}
		~FrustumVisibleChecker() = default;

		inline bool checkVisible(const BoundingVolume* volume, const glm::mat4& wtr) const {
			return volume->checkFrustum(_frustum, wtr);
		}
	private:
		const Frustum* _frustum = nullptr;
	};

	template<typename V>
	struct RenderListEmplacer {
		inline static bool _(H_Node* node, RenderList& list, const bool dirtyVisible, const uint8_t visibleId, V&& visibleChecker) {
			if (NodeUpdater::_<V>(node, dirtyVisible, visibleId, std::forward<V>(visibleChecker))) {
				if (RenderObject* renderObject = node->value().getRenderObject()) {
					renderObject->setNeedUpdate(true);
					list.addDescriptor(renderObject->getRenderDescriptor());
				}
				return true;
			}

			return false;
		}
	};

	template<typename V = EmptyVisibleChecker>
	inline void reloadRenderList(RenderList& list, H_Node* node, const bool dirtyVisible, const uint8_t visibleId, V&& visibleChecker = V()) {
		using list_emplacer_type = RenderListEmplacer<V>;
		list.clear();
		node->execute_with<list_emplacer_type>(list, dirtyVisible, visibleId, std::forward<V>(visibleChecker));
		list.sort();
	}

	template<typename V = EmptyVisibleChecker>
	inline void reloadRenderList(RenderList& list, H_Node** node, size_t count, const bool dirtyVisible, const uint8_t visibleId, V&& visibleChecker = V()) {
		using list_emplacer_type = RenderListEmplacer<V>;
		list.clear();
		for (size_t i = 0; i < count; ++i) {
			node[i]->execute_with<list_emplacer_type>(list, dirtyVisible, visibleId, std::forward<V>(visibleChecker));
		}
		list.sort();
	}

}