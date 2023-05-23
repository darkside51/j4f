#include "TextureHandler.h"

#include "../../Core/Engine.h"
#include "../Graphics.h"
#include "../Vulkan/vkRenderer.h"

namespace engine {

    TextureHandler::~TextureHandler() {
        Engine::getInstance().getModule<Graphics>()->getRenderer()->markToDelete(std::move(_texture));
    }

    uint32_t TextureHandler::_decrease_counter() noexcept {
        auto const count = m_counter.fetch_sub(1u, std::memory_order_release) - 1u;
        if (count == 1u) {
            // todo: add cache and remove texture from cache and remove markToDelete from here
            Engine::getInstance().getModule<Graphics>()->getRenderer()->markToDelete(std::move(_texture));
        }
        return count;
    }

}