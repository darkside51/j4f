#pragma once

#include "../Vulkan/vkTexture.h"
#include "../../Core/Linked_ptr.h"

#include <atomic>
#include <cstdint>

namespace engine {

    class TextureHandler {
        using type = TextureHandler;
        using counter_type = std::atomic_uint32_t;
        using texture_type = vulkan::VulkanTexture;
    public:
        template <typename ...Args>
        TextureHandler(Args&&...args) : _texture(std::forward<Args>(args)...) {}
        ~TextureHandler();

        uint32_t _decrease_counter() noexcept;
        inline uint32_t _increase_counter() noexcept { return m_counter.fetch_add(1u, std::memory_order_release) + 1u; }
        [[nodiscard]] inline uint32_t _use_count() const noexcept { return m_counter.load(std::memory_order_relaxed); }

        texture_type* texture() noexcept { return &_texture; }
        const texture_type* texture() const noexcept { return &_texture; }

    private:
        texture_type _texture;
        std::atomic_uint32_t m_counter = 0u;
    };

    using TexturePtr = linked_ptr<TextureHandler>;
}