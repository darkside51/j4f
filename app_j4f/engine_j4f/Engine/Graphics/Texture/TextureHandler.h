#pragma once

#include "../Vulkan/vkTexture.h"
#include "../../Core/Linked_ptr.h"

#include <atomic>
#include <cstdint>
#include <string_view>

namespace engine {

    class TextureHandler {
        friend class TextureCache;
        using type = TextureHandler;
        using counter_type = std::atomic_uint32_t;
        using texture_type = vulkan::VulkanTexture;
    public:
        template <typename ...Args>
        TextureHandler(Args&&...args) : m_texture(std::forward<Args>(args)...) {}
        ~TextureHandler();

        uint32_t _decrease_counter() noexcept;
        inline uint32_t _increase_counter() noexcept { return m_counter.fetch_add(1u, std::memory_order_release) + 1u; }
        [[nodiscard]] inline uint32_t _use_count() const noexcept { return m_counter.load(std::memory_order_relaxed); }

        texture_type* texture() noexcept { return &m_texture; }
        const texture_type* texture() const noexcept { return &m_texture; }

    private:
        texture_type m_texture;
        std::atomic_uint32_t m_counter = 0u;
        std::string_view m_key;

        enum class Flags : uint8_t {
            None = 0,
            Cached = 1,
            ForeverInCache = 2
        };

        Flags m_flags = Flags::None;
    };

    using TexturePtr = linked_ptr<TextureHandler>;
}