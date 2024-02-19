#include "RenderList.h"
#include <algorithm>

namespace engine {
	
	void RenderList::addEntity(RenderedEntity* e, const uint16_t layer) {
        if (e == nullptr) return;
		if (layer >= _entities.size()) _entities.resize(layer + 1u);
        _entities[layer].push_back(e);
	}

	void RenderList::clear() {
		_entities.clear();
	}

	void RenderList::eraseLayersData() {
		for (auto&& vec : _entities) {
			//memset(&vec[0], 0, vec.size() * sizeof(RenderDescriptor*)); // think about it
			const size_t sz = vec.size();
			vec.clear();
			vec.reserve(sz);
		}
	}

	void RenderList::sort() {
		for (auto&& vec : _entities) {
			std::sort(vec.begin(), vec.end(),
                      [](const RenderedEntity* a, const RenderedEntity* b) {
                return a->getRenderDescriptor().order < b->getRenderDescriptor().order;
            });
		}
	}
}