#include "RenderList.h"
#include "RenderDescriptor.h"
#include "RenderHelper.h"
#include "AutoBatchRender.h"

#include <algorithm>

namespace engine {
	
	void RenderList::addDescriptor(RenderDescriptor* d, const uint16_t layer) {
		if (layer >= _descriptors.size()) _descriptors.resize(layer + 1);
		_descriptors[layer].push_back(d);
	}

	void RenderList::clear() {
		_descriptors.clear();
	}

	void RenderList::eraseLayersData() {
		for (auto&& vec : _descriptors) {
			//memset(&vec[0], 0, vec.size() * sizeof(RenderDescriptor*)); // think about it
			const size_t sz = vec.size();
			vec.clear();
			vec.reserve(sz);
		}
	}

	void RenderList::sort() {
		for (auto&& vec : _descriptors) {
			std::sort(vec.begin(), vec.end(), [](const RenderDescriptor* a, const RenderDescriptor* b) { return a->order < b->order; });
		}
	}

    template<typename VP>
    inline void renderList(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, VP&& viewParams,
                std::vector<std::vector<RenderDescriptor*>>& descriptors) {
        auto&& renderHelper = Engine::getInstance().getModule<Graphics>()->getRenderHelper();
        auto&& autoBatcher = renderHelper->getAutoBatchRenderer();

        for (auto&& vec : descriptors) { // get layers and draw it
            for (auto&& descriptor : vec) {
                if (descriptor->visible) {
                    descriptor->render(commandBuffer, currentFrame, viewParams);
                }
            }
            autoBatcher->draw(commandBuffer, currentFrame);
        }
    }

	void RenderList::render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const ViewParams& viewParams) {
        renderList(commandBuffer, currentFrame, viewParams, _descriptors);
	}

	void RenderList::render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, ViewParams&& viewParams) {
        renderList(commandBuffer, currentFrame, std::move(viewParams), _descriptors);
	}
}