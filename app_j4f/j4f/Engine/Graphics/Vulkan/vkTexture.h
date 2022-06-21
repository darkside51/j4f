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
		VulkanTexture(VulkanRenderer* renderer, const uint32_t w, const uint32_t h, const uint32_t d = 1) : _renderer(renderer), _width(w), _height(h), _depth(d), _img(nullptr), _sampler(VK_NULL_HANDLE), _descriptor(VK_NULL_HANDLE), _generationState(VulkanTextureCreationState::UNKNOWN), _imageLayout(VK_IMAGE_LAYOUT_MAX_ENUM) {}
		VulkanTexture(
			VulkanRenderer* renderer,
			VkImageLayout layout,
			VkDescriptorSet readyDescriptorSet,
			VulkanImage* img,
			VkSampler sampler,
			const uint32_t w,
			const uint32_t h,
			const uint32_t d = 1
		) : _renderer(renderer), _width(w), _height(h), _depth(d), _img(img), _descriptor(readyDescriptorSet), _sampler(sampler), _generationState(VulkanTextureCreationState::CREATION_COMPLETE), _imageLayout(layout) {}
		~VulkanTexture();

		VulkanBuffer* generateWithData(const void* data, const VkFormat format, const uint8_t bpp, const bool createMipMaps);
		void create(const void* data, const VkFormat format, const uint8_t bpp, const bool createMipMaps, const bool deffered = false);
		void createSingleDescriptor(const VkImageLayout imageLayout, const uint32_t binding);

		const VkImageView getImageView() const;
		inline void setSampler(VkSampler sampler) { _sampler = sampler; }
		inline const VkSampler getSampler() const { return _sampler; }
		inline const VkDescriptorSet getSingleDescriptor() const { return _descriptor; }

		void fillGpuData(VulkanBuffer* staging, VulkanCommandBuffer& cmdBuffer);

		inline VulkanTextureCreationState generationState() const { return _generationState.load(std::memory_order_consume); }
		inline void noGenerate() { _generationState.store(VulkanTextureCreationState::NO_CREATED, std::memory_order_release); }

		void setImage(VulkanImage* img) { _img = img; }

	private:

		void generateMipMaps(VulkanCommandBuffer& cmdBuffer) const;
		
		VulkanRenderer* _renderer;

		uint32_t _width;
		uint32_t _height;
		uint32_t _depth;

		VulkanImage* _img;
		VkSampler _sampler;

		VkDescriptorSet _descriptor;

		std::atomic<VulkanTextureCreationState> _generationState;
		VkImageLayout _imageLayout;
		uint32_t _binding;
	};
}