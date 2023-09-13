#include "CascadeShadowMap.h"
#include "../../GpuProgramsManager.h"
#include "../../Graphics.h"
#include "../../Render/CommonVertexTypes.h"
#include "../../Vulkan/vkRenderer.h"
#include "../../Vulkan/vkHelper.h"
#include "../../Vulkan/vkTexture.h"

// todo: read this
// https://docs.microsoft.com/ru-ru/windows/win32/dxtecharts/common-techniques-to-improve-shadow-depth-maps

namespace engine {

	class Mesh;

	CascadeShadowMap::CascadeFrameBuffer::~CascadeFrameBuffer() {
		auto&& renderer = Engine::getInstance().getModule<Graphics>().getRenderer();
		destroy(renderer->getDevice()->device);
	}

	void CascadeShadowMap::initPipelines() { // init special pipelines
		using namespace vulkan;

		VertexDescription vertexDescription;
		vertexDescription.bindings_strides.push_back(std::make_pair(0, sizeof(TexturedVertex)));
		vertexDescription.attributes = TexturedVertex::getVertexAttributesDescription();

		VulkanPrimitiveTopology primitiveTopology = { PrimitiveTopology::TRIANGLE_LIST , false };
		VulkanRasterizationState rasterisation(CullMode::CULL_MODE_NONE, PoligonMode::POLYGON_MODE_FILL);
		VulkanDepthState depthState(true, true, VK_COMPARE_OP_LESS);
		VulkanStencilState stencilState(false);

		auto&& renderer = Engine::getInstance().getModule<Graphics>().getRenderer();
		auto&& gpuProgramManager = Engine::getInstance().getModule<Graphics>().getGpuProgramsManager();

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
			std::vector<engine::ProgramStageInfo> psi_shadow_plane;
			psi_shadow_plane.emplace_back(ProgramStage::VERTEX, "resources/shaders/shadows_plane.vsh.spv");
			psi_shadow_plane.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/shadows_plane.psh.spv");
			VulkanGpuProgram* program = gpuProgramManager->getProgram(psi_shadow_plane);

			_specialPipelines[static_cast<uint8_t>(ShadowMapSpecialPipelines::SH_PIPELINE_PLANE)] = renderer->getGraphicsPipeline(
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
		auto&& gpuProgramManager = Engine::getInstance().getModule<Graphics>().getGpuProgramsManager();

        const bool useGeometryShader = ((preferredTechnique == ShadowMapTechnique::SMT_AUTO || preferredTechnique == ShadowMapTechnique::SMT_GEOMETRY_SH) &&
                                        Engine::getInstance().getModule<Graphics>().getRenderer()->getDevice()->enabledFeatures.geometryShader);

		{
			std::vector<ProgramStageInfo> infos;
			infos.emplace_back(ProgramStage::VERTEX, "resources/shaders/mesh_skin_depthpass.vsh.spv");
			infos.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/mesh_depthpass.psh.spv");
			if (useGeometryShader) {
				infos.emplace_back(ProgramStage::GEOMETRY, "resources/shaders/mesh_depthpass.gsh.spv");
			}

			vulkan::VulkanGpuProgram* program = gpuProgramManager->getProgram(infos);
			registerShadowProgram<MeshSkinnedShadow>(program);
		}

		{
			std::vector<ProgramStageInfo> infos;
			infos.emplace_back(ProgramStage::VERTEX, "resources/shaders/mesh_depthpass.vsh.spv");
			infos.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/mesh_depthpass.psh.spv");
			if (useGeometryShader) {
				infos.emplace_back(ProgramStage::GEOMETRY, "resources/shaders/mesh_depthpass.gsh.spv");
			}

			vulkan::VulkanGpuProgram* program = gpuProgramManager->getProgram(infos);
			registerShadowProgram<MeshStaticShadow>(program);
		}

		{
			std::vector<ProgramStageInfo> infos;
			infos.emplace_back(ProgramStage::VERTEX, "resources/shaders/mesh_depthpass_instance.vsh.spv");
			infos.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/mesh_depthpass.psh.spv");
			if (useGeometryShader) {
				infos.emplace_back(ProgramStage::GEOMETRY, "resources/shaders/mesh_depthpass.gsh.spv");
			}

			vulkan::VulkanGpuProgram* program = gpuProgramManager->getProgram(infos);
			registerShadowProgram<MeshStaticInstanceShadow>(program);
		}
	}

	CascadeShadowMap::CascadeShadowMap(const uint16_t dim, const uint8_t textureBits, const uint8_t count,
                                       const vec2f& nearFar, const float minZ, const float maxZ) :
		_dimension(dim),
		_targetBits(textureBits),
		_cascadesCount(count),
		_cascadeSplits(count),
		_splitDepths(count),
		_cascadeViewProjects(count),
		_cascadeFrustums(count)
	{
        // get technique
        switch (preferredTechnique) {
            case ShadowMapTechnique::SMT_GEOMETRY_SH:
                if (Engine::getInstance().getModule<Graphics>().getRenderer()->getDevice()->enabledFeatures.geometryShader) {
                    _technique = ShadowMapTechnique::SMT_GEOMETRY_SH;
                    _cascades.resize(1u);
                } else {
                    _technique = ShadowMapTechnique::SMT_DEFAULT;
                    _cascades.resize(count);
                }
                break;
            case ShadowMapTechnique::SMT_INSTANCE_DRAW: {
                if (Engine::getInstance().getModule<Graphics>().getRenderer()->getDevice()->extensionSupported(
                        VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME)) {
                    _technique = ShadowMapTechnique::SMT_INSTANCE_DRAW;
                    _cascades.resize(1u);
                } else {
                    _technique = ShadowMapTechnique::SMT_DEFAULT;
                    _cascades.resize(count);
                }
            }
                break;
            case ShadowMapTechnique::SMT_AUTO:
                if (Engine::getInstance().getModule<Graphics>().getRenderer()->getDevice()->extensionSupported(
                        VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME)) {
                    _technique = ShadowMapTechnique::SMT_INSTANCE_DRAW;
                    _cascades.resize(1u);
                } else if (Engine::getInstance().getModule<Graphics>().getRenderer()->getDevice()->enabledFeatures.geometryShader) {
                    _technique = ShadowMapTechnique::SMT_GEOMETRY_SH;
                    _cascades.resize(1u);
                } else {
                    _technique = ShadowMapTechnique::SMT_DEFAULT;
                    _cascades.resize(count);
                }
                break;
            default:
                _technique = ShadowMapTechnique::SMT_DEFAULT;
                _cascades.resize(count);
                break;
        }

		initVariables();
		initCascadeSplits(nearFar, minZ, maxZ);

		_specialisationEntry[0u].constantID = 0u;
		_specialisationEntry[0u].offset = 0u;
		_specialisationEntry[0u].size = sizeof(uint32_t);

		_specialisationInfo.mapEntryCount = 1u;
		_specialisationInfo.dataSize = sizeof(uint32_t);
		_specialisationInfo.pData = &_cascadesCount;
		_specialisationInfo.pMapEntries = _specialisationEntry.data();
	}

	CascadeShadowMap::~CascadeShadowMap() {
		auto&& renderer = Engine::getInstance().getModule<Graphics>().getRenderer();
		renderer->getDevice()->destroyRenderPass(_depthRenderPass);
		delete _shadowMapTexture;
	}

	void CascadeShadowMap::initVariables() {
		_shadowClearValues.depthStencil = { 1.0f, 0 };

		auto&& renderer = Engine::getInstance().getModule<Graphics>().getRenderer();
		const auto depthFormat = renderer->getDevice()->getSupportedDepthFormat(_targetBits);

		// create render pass
		std::vector<VkAttachmentDescription> attachments(1);
		attachments[0] = vulkan::createAttachmentDescription(
			depthFormat,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
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

		depthImage->createImageView(VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_DEPTH_BIT, { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY });

		auto depthSampler = renderer->getSampler(
			VK_FILTER_LINEAR,
			VK_FILTER_LINEAR,
			VK_SAMPLER_MIPMAP_MODE_NEAREST,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE?
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE?
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE?
			VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
            0.0f,
			VK_COMPARE_OP_LESS						// to enable sampler2DShadow and sampler2DArrayShadow works
		);

		////// depthImageDescriptorSet
		const uint32_t binding = 0u;
		VkDescriptorSetLayoutBinding bindingLayout = { binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
		VkDescriptorSetLayout descriptorSetLayout = renderer->getDevice()->createDescriptorSetLayout({ bindingLayout }, nullptr);
		const auto depthImageDescriptorSet = renderer->allocateSingleDescriptorSetFromGlobalPool(descriptorSetLayout);
		renderer->bindImageToSingleDescriptorSet(depthImageDescriptorSet.first, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, depthSampler, depthImage->view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, binding);
		renderer->getDevice()->destroyDescriptorSetLayout(descriptorSetLayout, nullptr); // destroy there or store it????

		// texture
		_shadowMapTexture = new vulkan::VulkanTexture(renderer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, depthImageDescriptorSet, depthImage, depthSampler, _dimension, _dimension, 1);

		switch (_technique) {
			case ShadowMapTechnique::SMT_GEOMETRY_SH:
            case ShadowMapTechnique::SMT_INSTANCE_DRAW:
			{
				/*VkImageViewCreateInfo imageViewCI = {};
				imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

				imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
				imageViewCI.format = depthFormat;
				imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				imageViewCI.subresourceRange.baseMipLevel = 0;
				imageViewCI.subresourceRange.levelCount = 1;
				imageViewCI.subresourceRange.baseArrayLayer = 0;
				imageViewCI.subresourceRange.layerCount = _cascadesCount;
				imageViewCI.image = depthImage->image;

				vkCreateImageView(renderer->getDevice()->device, &imageViewCI, nullptr, &_cascades[0].view);

				_cascades[0].frameBuffer = new vulkan::VulkanFrameBuffer(
					renderer->getDevice(),
					_dimension,
					_dimension,
					_cascadesCount,
					_depthRenderPass,
					&_cascades[0].view,
					1
				);*/
				_cascades[0].frameBuffer = new vulkan::VulkanFrameBuffer(
					renderer->getDevice(),
					_dimension,
					_dimension,
					_cascadesCount,
					_depthRenderPass,
					&depthImage->view,
					1
				);
			}
				break;
			default:
			{
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
				break;
		}
	}

	void CascadeShadowMap::initCascadeSplits(const vec2f& nearFar, const float minZ, const float maxZ) {
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
		const vec2f& nearFar = camera->getNearFar();
		const float clipRange = nearFar.y - nearFar.x;

		// calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < _cascadesCount; ++i) {
			float splitDist = _cascadeSplits[i];

			vec3f frustumCorners[8] = {
				vec3f(-1.0f,  1.0f, 0.0f),
				vec3f(1.0f,  1.0f, 0.0f),
				vec3f(1.0f, -1.0f, 0.0f),
				vec3f(-1.0f, -1.0f,	0.0f),
				vec3f(-1.0f,  1.0f, 1.0f),
				vec3f(1.0f,  1.0f, 1.0f),
				vec3f(1.0f, -1.0f, 1.0f),
				vec3f(-1.0f, -1.0f, 1.0f)
			};

			// project frustum corners into world space
			const mat4f& invCam = const_cast<Camera*>(camera)->getInvTransform();
			for (uint32_t i = 0; i < 8; ++i) {
				vec4f invCorner = invCam * vec4f(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; ++i) {
				vec3f dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			// get frustum center
			vec3f frustumCenter = frustumCorners[0];
			for (uint32_t i = 1; i < 8; ++i) { frustumCenter += frustumCorners[i]; }
			frustumCenter /= 8.0f;

			// ��� ���������� ������� ���������� ����� ���������� ���������� �� ������ �� ������ ��������
			// �.�. �������� �����������, � �� ���������� frustumCenter � �������� - �� ���������� ����� ����������
			// �� ������ �� ��������� ������� ��������
			const float radius = vec_length(frustumCorners[7] - frustumCenter) * _frustumRadiusLambda;

			mat4f lightViewMatrix = glm::lookAt(frustumCenter - _lightDirection * radius * _cascadeLigthLambda, frustumCenter, vec3f(0.0f, 1.0f, 0.0f));
			mat4f lightOrthoMatrix = glm::ortho(-radius, radius, -radius, radius, -2.0f * radius, 2.0f * radius);

			// store split distance and matrix in cascade
			_splitDepths[i] = (nearFar.x + splitDist * clipRange) * -1.0f;
			_cascadeViewProjects[i] = lightOrthoMatrix * lightViewMatrix;
			_cascadeFrustums[i].calculate(_cascadeViewProjects[i]);

			lastSplitDist = _cascadeSplits[i];
		}

		_dirtyCascades = Engine::getInstance().getModule<Graphics>().getRenderer()->getSwapchainImagesCount();
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

	void CascadeShadowMap::updateShadowUniforms(vulkan::VulkanGpuProgram* program, const mat4f& cameraMatrix) {
		auto it = _programsShadowUniforms.find(program);
		if (it != _programsShadowUniforms.end()) {
			const ShadowUniforms& uniforms = it->second;
			program->setValueToLayout(uniforms.viewLayout, &const_cast<mat4f&>(cameraMatrix), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, false);
			program->setValueToLayout(uniforms.cascadeMatrixLayout, _cascadeViewProjects.data(), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, false);
			program->setValueToLayout(uniforms.cascadeSplitLayout, getSplitDepthsPointer<vec4f>(), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, false);
		} else {
			ShadowUniforms uniforms;
			uniforms.viewLayout = program->getGPUParamLayoutByName("view");
			uniforms.cascadeMatrixLayout = program->getGPUParamLayoutByName("cascade_matrix");
			uniforms.cascadeSplitLayout = program->getGPUParamLayoutByName("cascade_splits");

			program->setValueToLayout(uniforms.viewLayout, &const_cast<mat4f&>(cameraMatrix), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, false);
			program->setValueToLayout(uniforms.cascadeMatrixLayout, _cascadeViewProjects.data(), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, false);
			program->setValueToLayout(uniforms.cascadeSplitLayout, getSplitDepthsPointer<vec4f>(), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, vulkan::VulkanGpuProgram::UNDEFINED, false);

			_programsShadowUniforms.insert({ program, uniforms });
		}
	}

	void CascadeShadowMap::renderShadows(vulkan::VulkanCommandBuffer& commandBuffer) {

	}
}