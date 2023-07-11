#include "TextureHandler.h"
#include "TextureCache.h"

#include "../../Core/Engine.h"
#include "../Graphics.h"
#include "../Vulkan/vkRenderer.h"

namespace engine {

    TextureCache& TextureHandler::evaluateCache() const noexcept {
        return *Engine::getInstance().getModule<CacheManager>()->getCache<TextureCache>();
    }

    TextureHandler::~TextureHandler() {
        Engine::getInstance().getModule<Graphics>()->getRenderer()->markToDelete(std::move(m_texture));
    }

    uint32_t TextureHandler::_decrease_counter() noexcept {
        auto const count = m_counter.fetch_sub(1u, std::memory_order_release) - 1u;
        if (count == 1u) {
            m_textureCache.onTextureFree(this);
        }
        return count;
    }

}