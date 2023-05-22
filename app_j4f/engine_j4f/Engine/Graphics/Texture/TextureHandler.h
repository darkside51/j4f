#pragma once

#include "../Vulkan/vkTexture.h"
#include "../../Core/Linked_ptr.h"

#include <atomic>
#include <cstdint>

namespace engine {

    class TextureHandler {
        using type = TextureHandler;
        using counter_type = std::atomic_uint32_t;
    public:
        template <typename ...Args>
        TextureHandler(Args&&...args) : _texture(std::forward<Args>(args)...) {}
        ~TextureHandler();

        uint32_t _decrease_counter() noexcept;
        inline uint32_t _increase_counter() noexcept { return m_counter.fetch_add(1, std::memory_order_release) + 1; }
        [[nodiscard]] inline uint32_t _use_count() const noexcept { return m_counter.load(std::memory_order_relaxed); }

        vulkan::VulkanTexture* texture() noexcept { return &_texture; }
        const vulkan::VulkanTexture* texture() const noexcept { return &_texture; }

    private:
        vulkan::VulkanTexture _texture;
        std::atomic_uint32_t m_counter = 0;
    };

    using TexturePtr = linked_ptr<TextureHandler>;
}