#pragma once

#include "../Vulkan/vkCommandBuffer.h"
#include "../../Core/Math/mathematic.h"

#include "RenderDescriptor.h"
#include "RenderedEntity.h"
#include "RenderHelper.h"
#include "AutoBatchRender.h"

#include <functional>
#include <vector>
#include <cstdint>

namespace engine {

    class RenderedEntity;
	struct RenderDescriptor;
	struct ViewParams;

    class Empty {};

	class RenderList {
	public:
        void addEntity(RenderedEntity* e, const uint16_t layer = 0u);

		void clear();
		void eraseLayersData();

		void sort();

        template <typename T = Empty, typename T2 = Empty>
        void render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame,
                                const ViewParams& viewParams, const uint16_t drawCount = 1u,
                                const T & callback = {}, const T2 & callback2 = {}) {
            renderList(commandBuffer, currentFrame, viewParams, _entities, drawCount, callback, callback2);
        }

        template <typename T = Empty, typename T2 = Empty>
        void render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame,
                                ViewParams&& viewParams, const uint16_t drawCount = 1u,
                                const T & callback = {}, const T2 & callback2 = {}) {
            renderList(commandBuffer, currentFrame, std::move(viewParams), _entities, drawCount, callback, callback2);
        }

	private:
        template<typename VP, typename T = Empty, typename T2 = Empty>
        inline static void renderList(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, VP&& viewParams,
                                      std::vector<std::vector<RenderedEntity*>>& entities, const uint16_t drawCount,
                                      const T & callback = {}, const T2 & callback2 = {}) {

            auto && renderHelper = Engine::getInstance().getModule<Graphics>().getRenderHelper();
            auto && autoBatcher = renderHelper->getAutoBatchRenderer();

            for (auto && vec : entities) { // get layers and draw it
                for (auto && entity : vec) {
                    auto & descriptor = entity->getRenderDescriptor();
                    if (descriptor.visible) {
                        if constexpr (!std::is_same_v<T, Empty> && !std::is_same_v<T2, Empty>) {
                            auto result = callback(entity);
                            descriptor.render(commandBuffer, currentFrame, viewParams, drawCount);
                            callback2(entity, result);
                            continue;
                        } else if constexpr (!std::is_same_v<T, Empty>) {
                            callback(entity);
                        }

                        descriptor.render(commandBuffer, currentFrame, viewParams, drawCount);
                    }
                }
                autoBatcher->draw(commandBuffer, currentFrame);
            }
        }

        std::vector<std::vector<RenderedEntity*>> _entities;
	};
}