#pragma once

#include "vkCommandBuffer.h"
#include <vulkan/vulkan.h>

#include <atomic>

namespace vulkan {

	class VulkanRenderer;
	struct VulkanImage;

	enum class VulkanTextureCreationState : uint8_t {
		UNKNOWN = 0,
		CREATION_STARTED = 1,
		CREATION_COMPLETE = 2,
		NO_CREATED = 3
	};

	class VulkanTexture { 
	public:
		VulkanTexture(VulkanRenderer* renderer, const uint32_t w, const uint32_t h, const uint32_t d = 1) noexcept : _renderer(renderer), _width(w), _height(h), _depth(d), _img(nullptr), _sampler(VK_NULL_HANDLE), _descriptor(VK_NULL_HANDLE), _generationState(VulkanTextureCreationState::UNKNOWN), _imageLayout(VK_IMAGE_LAYOUT_MAX_ENUM), _arrayLayers(1) {}
		VulkanTexture(
			VulkanRenderer* renderer,
			VkImageLayout layout,
			const std::pair<VkDescriptorSet, uint32_t>& readyDescriptorSet,
			VulkanImage* img,
			VkSampler sampler,
			const uint32_t w,
			const uint32_t h,
			const uint32_t d = 1
		) noexcept : _renderer(renderer), _width(w), _height(h), _depth(d), _img(img), _descriptor(readyDescriptorSet.first), _descriptorPoolId(readyDescriptorSet.second), _sampler(sampler), _generationState(VulkanTextureCreationState::CREATION_COMPLETE), _imageLayout(layout), _arrayLayers(1) {}

        VulkanTexture(VulkanTexture&& t) noexcept :
            _renderer(t._renderer),
            _width(t._width),
            _height(t._height),
            _depth(t._depth),
            _img(t._img),
            _sampler(std::move(t._sampler)),
            _descriptor(t._descriptor),
            _descriptorPoolId(t._descriptorPoolId),
            _imageLayout(t._imageLayout),
            _binding(t._binding),
            _arrayLayers(t._arrayLayers) {
            _generationState = t._generationState.load(std::memory_order_relaxed);
            t._img = nullptr;
            t._descriptor = nullptr;
        }

        VulkanTexture& operator= (VulkanTexture&& t) noexcept {
            if (&t == this) return *this;

            _renderer = t._renderer;
            _width = t._width;
            _height = t._height;
            _depth = t._depth;
            _img = t._img;
            _sampler = std::move(t._sampler);
            _descriptor = t._descriptor;
            _descriptorPoolId = t._descriptorPoolId;
            _imageLayout = t._imageLayout;
            _binding = t._binding;
            _arrayLayers = t._arrayLayers ;
            _generationState = t._generationState.load(std::memory_order_relaxed);
            t._img = nullptr;
            t._descriptor = nullptr;
            return *this;
        }

        VulkanTexture(const VulkanTexture& t) = delete;
        VulkanTexture& operator= (const VulkanTexture& t) = delete;

        ~VulkanTexture();

		VulkanBuffer* generateWithData(const void** data, const uint32_t count, const VkFormat format, const uint8_t bpp, const bool createMipMaps, const VkImageViewType forceType);

		void create(const void* data, const VkFormat format, const uint8_t bpp, const bool createMipMaps, const bool deffered = false, const VkImageViewType forceType = VK_IMAGE_VIEW_TYPE_MAX_ENUM);
		void create(const void** data, const uint32_t layerCount, const VkFormat format, const uint8_t bpp, const bool createMipMaps, const bool deffered = false, const VkImageViewType forceType = VK_IMAGE_VIEW_TYPE_MAX_ENUM);

		void createSingleDescriptor(const VkImageLayout imageLayout, const uint32_t binding);

		[[nodiscard]] VkImageView getImageView() const;
		inline void setSampler(VkSampler sampler) { _sampler = sampler; }
        [[nodiscard]] inline VkSampler getSampler() const { return _sampler; }
        [[nodiscard]] inline VkDescriptorSet getSingleDescriptor() const { return _descriptor; }

		void fillGpuData(VulkanBuffer* staging, VulkanCommandBuffer& cmdBuffer, const uint32_t baseLayer, const uint32_t layerCount);

        [[nodiscard]] inline VulkanTextureCreationState generationState() const { return _generationState.load(std::memory_order_consume); }
		inline void noGenerate() { _generationState.store(VulkanTextureCreationState::NO_CREATED, std::memory_order_release); }

		void setImage(VulkanImage* img) { _img = img; }

	private:

		void generateMipMaps(VulkanCommandBuffer& cmdBuffer) const;
		
		VulkanRenderer* _renderer = nullptr;

		uint32_t _width = 0;
		uint32_t _height = 0;
		uint32_t _depth = 0;

		VulkanImage* _img = nullptr;
		VkSampler _sampler;

		VkDescriptorSet _descriptor = VK_NULL_HANDLE;
		uint32_t _descriptorPoolId = 0;

		std::atomic<VulkanTextureCreationState> _generationState;
		VkImageLayout _imageLayout;
		uint32_t _binding = 0;
		uint32_t _arrayLayers = 0;
	};
}