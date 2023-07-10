#pragma once

#include "../../Core/AssetManager.h"
#include "../../Core/Linked_ptr.h"

#include "TextureData.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <atomic>

namespace engine {
    class TextureHandler;
    using TexturePtr = linked_ptr<TextureHandler>;

    template<>
    struct AssetLoadingParams<TextureHandler> : public AssetLoadingFlags {
        struct TextureFlags {
            uint8_t deffered : 1;
            uint8_t useMipMaps : 1;
        };

        union TextureLoadingFlags {
            uint8_t mask;
            TextureFlags flags;

            TextureLoadingFlags() : mask(0b00000010) {}
            explicit TextureLoadingFlags(const uint8_t f) : mask(f) {}

            TextureFlags* operator->() { return &flags; }
            const TextureFlags* operator->() const { return &flags; }
        };

        std::vector<std::string> files;
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        uint32_t binding = 0;

        TextureData *texData = nullptr;
        TextureFormatType formatType = TextureFormatType::UNORM;
        VkImageViewType imageViewTypeForce = VkImageViewType::VK_IMAGE_VIEW_TYPE_MAX_ENUM;

        TextureLoadingFlags textureFlags;

        std::string cacheName;
    };

    using TexturePtrLoadingParams = AssetLoadingParams<TextureHandler>;
    using TexturePtrLoadingCallback = AssetLoadingCallback<TexturePtr>;

    class TexturePtrLoader {
    public:
        using asset_type = TexturePtr;
        static void loadAsset(asset_type& v, const TexturePtrLoadingParams& params, const TexturePtrLoadingCallback& callback);
    private:
        static void addCallback(asset_type, const TexturePtrLoadingCallback&);
        static void executeCallbacks(asset_type, const AssetLoadingResult);

        static asset_type createTexture(const TexturePtrLoadingParams& params, const TexturePtrLoadingCallback& callback);

        inline static std::atomic_bool _callbacksLock;
        inline static std::unordered_map<asset_type, std::vector<TexturePtrLoadingCallback>> _callbacks;
    };
}