#include "CascadeShadowMap.h"
#include "../Camera.h"
#include "../../GpuProgramsManager.h"
#include "../../Graphics.h"
#include "../../Render/CommonVertexTypes.h"
#include "../../Vulkan/vkRenderer.h"
#include "../../Vulkan/vkHelper.h"
#include "../../Vulkan/vkTexture.h"
#include "../../../Core/Engine.h"

// todo: read this
// https://docs.microsoft.com/ru-ru/windows/win32/dxtecharts/common-techniques-to-improve-shadow-depth-maps

namespace engine {

	class Mesh;

	CascadeShadowMap::CascadeFrameBuffer::~CascadeFrameBuffer() {
		auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();
		destroy(renderer->getDevice()->device);
	}

	void CascadeShadowMap::initPipelines() { // init special pipelines
		using namespace vulkan;

		std::vector<VkVertexInputAttributeDescription> vertexInputAttributs = std::move(TexturedVertex::getVertexAttributesDescription());
		VertexDescription vertexDescription;
		vertexDescription.bindings_strides.push_back(std::make_pair(0, sizeof(TexturedVertex)));
		vertexDescription.attributesCount = static_cast<uint32_t>(vertexInputAttributs.size());
		vertexDescription.attributes = vertexInputAttributs.data();

		VulkanPrimitiveTopology primitiveTopology = { TRIANGLE_LIST , false };
		VulkanRasterizationState rasterisation(CULL_MODE_NONE, POLYGON_MODE_FILL);
		VulkanDepthState depthState(true, true, VK_COMPARE_OP_LESS);
		VulkanStencilState stencilState(false);

		auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();
		auto&& gpuProgramManager = Engine::getInstance().getModule<Graphics>()->getGpuProgramsManager();

		{
			std::vector<engine::ProgramStageInfo> psi_shadow_debug;
			psi_shadow_debug.emplace_back(ProgramStage::VERTEX, "resources/shaders/texture.vsh.spv");
			psi_shadow_debug.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/texture_shadow.psh.spv");
			VulkanGpuProgram* program = gpuProgramManager->getProgram(psi_shadow_debug);

			_specialPipelines[static_cast<uint8_t>(ShadowMapSpecialPipelines::SH_PIPELINE_DEBUG)] = renderer->getGraphicsPipeline(
				vertexDescription,
				primitiveTopology,
				rasterisation,
				CommonBlendModes::blend_alpha,
				depthState,
				stencilState,
				program
			);
		}

		{
			std::vector<engine::ProgramStageInfo> psi_shadow_plain;
			psi_shadow_plain.emplace_back(ProgramStage::VERTEX, "resources/shaders/shadows_plain.vsh.spv");
			psi_shadow_plain.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/shadows_plain.psh.spv");
			VulkanGpuProgram* program = gpuProgramManager->getProgram(psi_shadow_plain);

			_specialPipelines[static_cast<uint8_t>(ShadowMapSpecialPipelines::SH_PIPEINE_PLAIN)] = renderer->getGraphicsPipeline(
				vertexDescription,
				primitiveTopology,
				rasterisation,
				CommonBlendModes::blend_alpha,
				depthState,
				stencilState,
				program
			);
		}
	}

	void CascadeShadowMap::registerCommonShadowPrograms() {
		auto&& gpuProgramManager = Engine::getInstance().getModule<Graphics>()->getGpuProgramsManager();

		{
			std::vector<ProgramStageInfo> psi_shadow;
			psi_shadow.emplace_back(ProgramStage::VERTEX, "resources/shaders/mesh_skin_depthpass.vsh.spv");
			psi_shadow.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/depthpass.psh.spv");
			vulkan::VulkanGpuProgram* program = gpuProgramManager->getProgram(psi_shadow);

			registerShadowProgram<Mesh>(program);
		}
	}

	CascadeShadowMap::~CascadeShadowMap() {
		auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();
		renderer->getDevice()->destroyRenderPass(_depthRenderPass);
		delete _shadowMapTexture;
	}

	void CascadeShadowMap::initVariables() {
		_shadowClearValues.depthStencil = { 1.0f, 0 };

		auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();
		const auto depthFormat = renderer->getDevice()->getSupportedDepthFormat();

		// create render pass
		std::vector<VkAttachmentDescription> attachments(1);
		attachments[0] = vulkan::createAttachmentDescription(
			depthFormat,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_SAMPLE_COUNT_1_BIT,
			0
		);

		std::vector<VkSubpassDependency> dependencies(2);

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		_depthRenderPass = vulkan::createRenderPass(renderer->getDevice(), VK_PIPELINE_BIND_POINT_GRAPHICS, dependencies, attachments);

		// create depthImage
		vulkan::VulkanImage* depthImage = new vulkan::VulkanImage(
			renderer->getDevice(),
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_TYPE_2D,
			depthFormat,
			1,
			_dimension,
			_dimension,
			1,
			0,
			_cascadesCount,
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_TILING_OPTIMAL
		);

		depthImage->createImageView(VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_DEPTH_BIT);

		auto depthSampler = renderer->getSampler(
			VK_FILTER_LINEAR,
			VK_FILTER_LINEAR,
			VK_SAMPLER_MIPMAP_MODE_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE?
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE?
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE?
			VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE
		);

		////// depthImageDescriptorSet
		uint32_t binding = 0;
		VkDescriptorSetLayoutBinding bindingLayout = { binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
		VkDescriptorSetLayout descriptorSetLayout = renderer->getDevice()->createDescriptorSetLayout({ bindingLayout }, nullptr);
		VkDescriptorSet depthImageDescriptorSet = renderer->allocateSingleDescriptorSetFromGlobalPool(descriptorSetLayout);
		renderer->bindImageToSingleDescriptorSet(depthImageDescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, depthSampler, depthImage->view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, binding);
		renderer->getDevice()->destroyDescriptorSetLayout(descriptorSetLayout, nullptr); // destroy there or store it????

		// texture
		_shadowMapTexture = new vulkan::VulkanTexture(renderer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, depthImageDescriptorSet, depthImage, depthSampler, _dimension, _dimension, 1);

		// one image and framebuffer per cascade
		for (uint8_t i = 0; i < _cascadesCount; ++i) {
			// image view for this cascade's layer (inside the depth map)
			// this view is used to render to that specific depth image layer
			VkImageViewCreateInfo imageViewCI = {};
			imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

			imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			imageViewCI.format = depthFormat;
			imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			imageViewCI.subresourceRange.baseMipLevel = 0;
			imageViewCI.subresourceRange.levelCount = 1;
			imageViewCI.subresourceRange.baseArrayLayer = i;
			imageViewCI.subresourceRange.layerCount = 1;
			imageViewCI.image = depthImage->image;

			vkCreateImageView(renderer->getDevice()->device, &imageViewCI, nullptr, &_cascades[i].view);

			_cascades[i].frameBuffer = new vulkan::VulkanFrameBuffer(
				renderer->getDevice(),
				_dimension,
				_dimension,
				1,
				_depthRenderPass,
				&_cascades[i].view,
				1
			);
		}
	}

	void CascadeShadowMap::initCascadeSplits(const glm::vec2& nearFar, const float minZ, const float maxZ) {
		const float clipRange = nearFar.y - nearFar.x;

		const float range = maxZ - minZ;
		const float ratio = maxZ / minZ;

		// calculate split depths based on view camera frustum
		// based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0; i < _cascadesCount; ++i) {
			const float p = static_cast<float>(i + 1) / static_cast<float>(_cascadesCount);
			const float log = minZ * std::pow(ratio, p);
			const float uniform = minZ + range * p;
			const float d = _cascadeSplitLambda * (log - uniform) + uniform;
			_cascadeSplits[i] = (d - nearFar.x) / clipRange;
		}
	}

	void CascadeShadowMap::updateCascades(const Camera* camera) {
		const glm::vec2& nearFar = camera->getNearFar();
		const float clipRange = nearFar.y - nearFar.x;

		// calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < _cascadesCount; ++i) {
			float splitDist = _cascadeSplits[i];

			glm::vec3 frustumCorners[8] = {
				glm::vec3(-1.0f,  1.0f, 0.0f),
				glm::vec3(1.0f,  1.0f, 0.0f),
				glm::vec3(1.0f, -1.0f, 0.0f),
				glm::vec3(-1.0f, -1.0f,	0.0f),
				glm::vec3(-1.0f,  1.0f, 1.0f),
				glm::vec3(1.0f,  1.0f, 1.0f),
				glm::vec3(1.0f, -1.0f, 1.0f),
				glm::vec3(-1.0f, -1.0f, 1.0f)
			};

			// project frustum corners into world space
			const glm::mat4& invCam = const_cast<Camera*>(camera)->getInvMatrix();
			for (uint32_t i = 0; i < 8; ++i) {
				glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; ++i) {
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			// get frustum center
			glm::vec3 frustumCenter = frustumCorners[0];
			for (uint32_t i = 1; i < 8; ++i) { frustumCenter += frustumCorners[i]; }
			frustumCenter /= 8.0f;

			// для вычисления радиуса достаточно взять наибольшее расстояние от центра до вершин пирамиды
			// т.к. пирамида расширяется, а оп построению frustumCenter в середине - до достаточно взять расстояние
			// от центра до последней вершины пирамиды
			const float radius = vec_length(frustumCorners[7] - frustumCenter) * _frustumRadiusLambda;

			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - _lightDirection * radius * _cascadeLigthLambda, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 lightOrthoMatrix = glm::ortho(-radius, radius, -radius, radius, -2.0f * radius, 2.0f * radius);

			// store split distance and matrix in cascade
			_splitDepths[i] = (nearFar.x + splitDist * clipRange) * -1.0f;
			_cascadeViewProjects[i] = lightOrthoMatrix * lightViewMatrix;

			lastSplitDist = _cascadeSplits[i];
		}

		_dirtyCascades = Engine::getInstance().getModule<Graphics>()->getRenderer()->getSwapchainImagesCount();
	}

	void CascadeShadowMap::registerProgramAsReciever(vulkan::VulkanGpuProgram* program) {
		auto it = _programsShadowUniforms.find(program);
		if (it == _programsShadowUniforms.end()) {
			ShadowUniforms uniforms;
			uniforms.viewLayout = program->getGPUParamLayoutByName("view");
			uniforms.cascadeMatrixLayout = program->getGPUParamLayoutByName("cascade_matrix");
			uniforms.cascadeSplitLayout = program->getGPUParamLayoutByName("cascade_splits");
			_programsShadowUniforms.insert({ program, uniforms });
		}
	}

	void CascadeShadowMap::unregisterProgramAsReciever(vulkan::VulkanGpuProgram* program) {
		auto it = _programsShadowUniforms.find(program);
		if (it != _programsShadowUniforms.end()) {
			_programsShadowUniforms.erase(it);
		}
	}

	void CascadeShadowMap::updateShadowUniforms(vulkan::VulkanGpuProgram* program, const glm::mat4& cameraMatrix) {
		auto it = _programsShadowUniforms.find(program);
		if (it != _programsShadowUniforms.end()) {
			const ShadowUniforms& uniforms = it->second;
			program->setValueToLayout(uniforms.viewLayout, &const_cast<glm::mat4&>(cameraMatrix), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, false);
			program->setValueToLayout(uniforms.cascadeMatrixLayout, _cascadeViewProjects.data(), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, false);
			program->setValueToLayout(uniforms.cascadeSplitLayout, getSplitDepthsPointer<glm::vec4>(), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, false);
		} else {
			ShadowUniforms uniforms;
			uniforms.viewLayout = program->getGPUParamLayoutByName("view");
			uniforms.cascadeMatrixLayout = program->getGPUParamLayoutByName("cascade_matrix");
			uniforms.cascadeSplitLayout = program->getGPUParamLayoutByName("cascade_splits");

			program->setValueToLayout(uniforms.viewLayout, &const_cast<glm::mat4&>(cameraMatrix), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, false);
			program->setValueToLayout(uniforms.cascadeMatrixLayout, _cascadeViewProjects.data(), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, false);
			program->setValueToLayout(uniforms.cascadeSplitLayout, getSplitDepthsPointer<glm::vec4>(), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, false);

			_programsShadowUniforms.insert({ program, uniforms });
		}
	}

	void CascadeShadowMap::renderShadows(vulkan::VulkanCommandBuffer& commandBuffer) {

	}
}