#pragma once

#include "../../../Core/Math/math.h"
#include "../../Vulkan/vkCommandBuffer.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

namespace vulkan {
	class VulkanTexture;
	struct VulkanFrameBuffer;
}

namespace engine {

	class Camera;

	class CascadeShadowMap {
		struct CascadeFrameBuffer {
			~CascadeFrameBuffer(); // call destroy from render->device

			inline void destroy(VkDevice device) {
				vkDestroyImageView(device, view, nullptr);
				delete frameBuffer;
			}

			vulkan::VulkanFrameBuffer* frameBuffer; // нужен ли отдельный под каждый swapchain image?
			VkImageView view;
		};

	public:

		CascadeShadowMap(const uint16_t dim, const uint8_t count) : 
			_dimension(dim), 
			_cascadesCount(count), 
			_cascades(count),
			_cascadeSplits(count),
			_splitDepths(count),
			_cascadeViewProjects(count)
		{
			initVariables();
		}

		~CascadeShadowMap();

		void initCascadeSplits(const float minZ, const float maxZ);
		inline void setLamdas(const float s, const float r, const float l) {
			_cascadeSplitLambda = s;
			_frustumRadiusLambda = r;
			_cascadeLigthLambda = l;
		}

		void updateCascades(const Camera* camera);
		void renderShadows(vulkan::VulkanCommandBuffer& commandBuffer);

		inline const VkRenderPass getRenderPass() const { return _depthRenderPass; }
		inline const vulkan::VulkanTexture* getTexture() const { return _shadowMapTexture; }

	private:
		void initVariables();

		uint8_t _cascadesCount;
		uint16_t _dimension;
		float _cascadeSplitLambda = 1.0f;
		float _frustumRadiusLambda = 1.0f;
		float _cascadeLigthLambda = 1.0f;

		std::vector<CascadeFrameBuffer> _cascades; // _cascadesCount
		std::vector<float> _cascadeSplits; // _cascadesCount
		std::vector<float> _splitDepths; // _cascadesCount
		std::vector<glm::mat4> _cascadeViewProjects; // _cascadesCount

		VkRenderPass _depthRenderPass;
		vulkan::VulkanTexture* _shadowMapTexture;
	};

}