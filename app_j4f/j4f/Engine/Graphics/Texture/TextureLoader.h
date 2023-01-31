#pragma once

#include "../../Core/AssetManager.h"

#include "TextureData.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <atomic>

namespace vulkan {
	class VulkanTexture;
}

namespace engine {

	template<>
	struct AssetLoadingParams<vulkan::VulkanTexture> : public AssetLoadingFlags {
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

	using TextureLoadingParams = AssetLoadingParams<vulkan::VulkanTexture>;
	using TextureLoadingCallback = AssetLoadingCallback<vulkan::VulkanTexture*>;

	class TextureLoader {
	public:
		using asset_type = vulkan::VulkanTexture*;
		static void loadAsset(vulkan::VulkanTexture*& v, const TextureLoadingParams& params, const TextureLoadingCallback& callback);
	private:
		static void addCallback(vulkan::VulkanTexture*, const TextureLoadingCallback&);
		static void executeCallbacks(vulkan::VulkanTexture*, const AssetLoadingResult);

		static vulkan::VulkanTexture* createTexture(const TextureLoadingParams& params, const TextureLoadingCallback& callback);

		inline static std::atomic_bool _callbacksLock;
		inline static std::unordered_map<vulkan::VulkanTexture*, std::vector<TextureLoadingCallback>> _callbacks;
	};

}