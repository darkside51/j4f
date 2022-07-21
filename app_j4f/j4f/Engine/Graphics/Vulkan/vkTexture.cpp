#include "vkTexture.h"
#include "vkImage.h"
#include "VkRenderer.h"

namespace vulkan {

	VulkanTexture::~VulkanTexture() {
		if (_img) { delete _img; }
		_img = nullptr;

		if (_descriptor) {
			_renderer->freeDescriptorSetsFromGlobalPool(&_descriptor, 1);
		}
	}

	void VulkanTexture::create(const void* data, const VkFormat format, const uint8_t bpp, const bool createMipMaps, const bool deffered) {
		_generationState.store(VulkanTextureCreationState::CREATION_STARTED, std::memory_order_release);
		VulkanBuffer* staging = generateWithData(data, format, bpp, createMipMaps);

		if (deffered) {
			_renderer->addDefferedGenerateTexture(this, staging, 0, 1);
		} else {
			auto&& cmdBuffer = _renderer->getSupportCommandBuffer();
			fillGpuData(staging, cmdBuffer, 0, 1);
			_renderer->addTmpBuffer(staging);
		}
	}

	VulkanBuffer* VulkanTexture::generateWithData(const void* data, const VkFormat format, const uint8_t bpp, const bool createMipMaps) {
		const size_t dataSize = _width * _height * (bpp / 8);
		const uint8_t mipLevels = (createMipMaps ? (static_cast<uint8_t>(std::floor(std::log2(std::max(_width, _height)))) + 1) : 1);

		VulkanBuffer* staging = new VulkanBuffer();
		_renderer->getDevice()->createBuffer(
			VK_SHARING_MODE_EXCLUSIVE,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			staging,
			dataSize
		);

		staging->upload(data, dataSize, 0);

		_img = new vulkan::VulkanImage(
			_renderer->getDevice(),
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_TYPE_2D,
			format,
			mipLevels,
			_width,
			_height
		);

		_img->createImageView(VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);

		if (_sampler == VK_NULL_HANDLE) { // трилинейная фильтрация + CLAMP_TO_EDGE по умолчанию
			/*_sampler = _renderer->getSampler(
				VK_FILTER_LINEAR,
				VK_FILTER_LINEAR,
				VK_SAMPLER_MIPMAP_MODE_LINEAR,
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
				VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK
			);*/

			/*_sampler = _renderer->getSampler(
				VK_FILTER_LINEAR,
				VK_FILTER_LINEAR,
				VK_SAMPLER_MIPMAP_MODE_LINEAR,
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK
			);*/

			_sampler = _renderer->getSampler(
				VK_FILTER_LINEAR,
				VK_FILTER_LINEAR,
				VK_SAMPLER_MIPMAP_MODE_LINEAR,
				VK_SAMPLER_ADDRESS_MODE_REPEAT,
				VK_SAMPLER_ADDRESS_MODE_REPEAT,
				VK_SAMPLER_ADDRESS_MODE_REPEAT,
				VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK
			);
		}

		return staging;
	}

	void VulkanTexture::fillGpuData(VulkanBuffer* staging, VulkanCommandBuffer& cmdBuffer, const uint32_t baseLayer, const uint32_t layerCount) {
		cmdBuffer.begin();

		// image memory barriers for the texture image
		cmdBuffer.changeImageLayout(
			*_img,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			0,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_HOST_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT);

		VkBufferImageCopy region;
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = baseLayer;
		region.imageSubresource.layerCount = layerCount;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { _width, _height, _depth };

		cmdBuffer.cmdCopyBufferToImage(*staging, *_img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		if (_img->mipLevels > 1) { // generate mipmaps
			generateMipMaps(cmdBuffer);
		} else {
			cmdBuffer.changeImageLayout(
				*_img,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		_generationState.store(VulkanTextureCreationState::CREATION_COMPLETE, std::memory_order_release);
		if (_imageLayout != VK_IMAGE_LAYOUT_MAX_ENUM) {
			createSingleDescriptor(_imageLayout, _binding);
		}
	}

	void VulkanTexture::generateMipMaps(VulkanCommandBuffer& cmdBuffer) const {
		VkImageMemoryBarrier barrier;
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.pNext = nullptr;
		barrier.image = _img->image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = _width;
		int32_t mipHeight = _height;

		for (uint32_t i = 1; i < _img->mipLevels; ++i) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			cmdBuffer.cmdPipelineBarrier(
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			cmdBuffer.cmdBlitImage(
				*_img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*_img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, VK_FILTER_LINEAR,
				{ 0, 0, 0 }, { mipWidth, mipHeight, 1 },
				{ 0, 0, 0 }, { mipWidth > 1 ? mipWidth >> 1 : 1, mipHeight > 1 ? mipHeight >> 1 : 1, 1 },
				{ VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1 },
				{ VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1 }
			);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			cmdBuffer.cmdPipelineBarrier(
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			if (mipWidth > 1) mipWidth = mipWidth >> 1;
			if (mipHeight > 1) mipHeight = mipHeight >> 1;
		}

		barrier.subresourceRange.baseMipLevel = _img->mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		cmdBuffer.cmdPipelineBarrier( // can change with cmdBuffer.changeImageLayout(это одно и то же, но там внутри создается новый объект VkImageMemoryBarrier каждый раз)
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}

	const VkImageView VulkanTexture::getImageView() const { return _img->view; }

	void VulkanTexture::createSingleDescriptor(const VkImageLayout imageLayout, const uint32_t binding) {
		if (_generationState.load(std::memory_order_consume) == VulkanTextureCreationState::CREATION_COMPLETE) {
			VkDescriptorSetLayoutBinding bindingLayout = { binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
			VkDescriptorSetLayout descriptorSetLayout = _renderer->getDevice()->createDescriptorSetLayout({ bindingLayout }, nullptr);
			_descriptor = _renderer->allocateSingleDescriptorSetFromGlobalPool(descriptorSetLayout);
			_renderer->bindImageToSingleDescriptorSet(_descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, _sampler, _img->view, imageLayout, binding);

			_renderer->getDevice()->destroyDescriptorSetLayout(descriptorSetLayout, nullptr); // destroy there or store it????
		} else {
			_imageLayout = imageLayout;
			_binding = binding;
		}
	}
}