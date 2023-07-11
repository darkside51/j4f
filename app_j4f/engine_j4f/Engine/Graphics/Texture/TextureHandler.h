#pragma once

#include "../Vulkan/vkTexture.h"
#include "../../Core/Linked_ptr.h"

#include <atomic>
#include <cstdint>
#include <string_view>

namespace engine {
    class TextureCache;

    class TextureHandler {
        friend class TextureCache;
        using counter_type = std::atomic_uint32_t;
        using value_type = vulkan::VulkanTexture;
    public:
        template <typename ...Args>
        explicit TextureHandler(Args&&...args) :
        m_texture(std::forward<Args>(args)...),
        m_textureCache(evaluateCache()) {
            evaluateCache();
        }

        ~TextureHandler();

        uint32_t _decrease_counter() noexcept;

        inline uint32_t _increase_counter() noexcept {
            return m_counter.fetch_add(1u, std::memory_order_release) + 1u;
        }

        [[nodiscard]] inline uint32_t _use_count() const noexcept {
            return m_counter.load(std::memory_order_relaxed);
        }

        value_type* get() noexcept { return &m_texture; }
        const value_type* get() const noexcept { return &m_texture; }

    private:
        TextureCache& evaluateCache() const noexcept;
        value_type m_texture;
        std::atomic_uint32_t m_counter = 0u;
        std::string_view m_key;
        TextureCache& m_textureCache;

        enum class Flags : uint8_t {
            None = 0u,
            Cached = 1u,
            ForeverInCache = 2u
        } m_flags = Flags::None;
    };

    using TexturePtr = linked_ptr<TextureHandler>;
}