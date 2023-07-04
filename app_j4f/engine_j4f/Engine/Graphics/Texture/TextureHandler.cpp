#include "TextureHandler.h"
#include "TextureCache.h"

#include "../../Core/Engine.h"
#include "../Graphics.h"
#include "../Vulkan/vkRenderer.h"

namespace engine {

    TextureHandler::~TextureHandler() {
        Engine::getInstance().getModule<Graphics>()->getRenderer()->markToDelete(std::move(m_texture));
    }

    uint32_t TextureHandler::_decrease_counter() noexcept {
        auto const count = m_counter.fetch_sub(1u, std::memory_order_release) - 1u;
        if (count == 1u) {
            if (auto&& cache = Engine::getInstance().getModule<CacheManager>()->getCache<TextureCache>()) {
                cache->onTextureFree(this);
            }
        }
        return count;
    }

}