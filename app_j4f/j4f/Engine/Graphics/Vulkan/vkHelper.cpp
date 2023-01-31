#include "vkHelper.h"
#include "vkDevice.h"

namespace vulkan {

	VkRenderPass createRenderPass(
		VulkanDevice* device,
		VkPipelineBindPoint pipelineBindPoint,
		const std::vector<VkSubpassDependency>& dependencies,
		const std::vector<VkAttachmentDescription>& attachments,
		const VkSubpassDescriptionFlags flags,
		const uint32_t inputAttachmentCount,
		const VkAttachmentReference* pInputAttachments,
		const VkAttachmentReference* pResolveAttachments,
		const uint32_t preserveAttachmentCount,
		const uint32_t* pPreserveAttachments) {

		// collect attachment references
		std::vector<VkAttachmentReference> colorReferences;
		VkAttachmentReference depthReference = {};
		bool hasDepth = false;
		bool hasColor = false;

		uint32_t attachmentIndex = 0;

		for (auto& attachment : attachments) {
			if (isDepthStencil(attachment.format)) {
				// only one depth attachment allowed
				assert(!hasDepth);
				depthReference.attachment = attachmentIndex;
				depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				hasDepth = true;
			} else {
				colorReferences.push_back({ attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
				hasColor = true;
			}
			++attachmentIndex;
		}

		VkSubpassDescription description;
		description.flags = flags;
		description.pipelineBindPoint = pipelineBindPoint;

		if (hasColor) {
			description.pColorAttachments = colorReferences.data();
			description.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
		} else {
			description.pColorAttachments = nullptr;
			description.colorAttachmentCount = 0;
		}
		
		if (hasDepth) {
			description.pDepthStencilAttachment = &depthReference;
		}

		description.inputAttachmentCount = inputAttachmentCount;
		description.pInputAttachments = pInputAttachments;

		description.pResolveAttachments = pResolveAttachments;

		description.preserveAttachmentCount = preserveAttachmentCount;
		description.pPreserveAttachments = pPreserveAttachments;

		std::vector<VkSubpassDescription> subpassDescriptions(1);
		subpassDescriptions[0] = description;

		VkRenderPass renderPass = device->createRenderPass(attachments, subpassDescriptions, dependencies);
		return renderPass;
	}
}