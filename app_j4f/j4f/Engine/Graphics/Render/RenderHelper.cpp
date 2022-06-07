#include "RenderHelper.h"

#include "../../Core/Engine.h"
#include "../Graphics.h"
#include "../GpuProgramsManager.h"
#include "../Vulkan/vkRenderer.h"
#include "../Vulkan/vkHelper.h"
#include "AutoBatchRender.h"

#include "../../Core/Math/math.h"

namespace engine {

	void VulkanStreamBuffer::createBuffer(const size_t sz) {
		constexpr size_t minBufferSize = 1024 * 100; // 100kb
		size = std::max(sz, minBufferSize);

		auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();
		renderer->getDevice()->createBuffer(
			VK_SHARING_MODE_EXCLUSIVE,
			usageFlags,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&vkBuffer,
			size
		);
	}

	//////////////////////////////////

	RenderHelper::RenderHelper(vulkan::VulkanRenderer* r, GpuProgramsManager* m) : _renderer(r), _gpuProgramManager(m), _autoBatchRenderer(new AutoBatchRenderer()) {
		const uint8_t swapChainImagesCount = _renderer->getSwapchainImagCount();
		_dynamic_vertices.resize(swapChainImagesCount);
		_dynamic_indices.resize(swapChainImagesCount);
	}

	RenderHelper::~RenderHelper() {
		_dynamic_vertices.clear();
		_dynamic_indices.clear();
		delete _autoBatchRenderer;
	}

	void RenderHelper::initCommonPipelines() {
		using namespace vulkan;

		std::vector<engine::ProgramStageInfo> psiColored;
		psiColored.emplace_back(ProgramStage::VERTEX, "resources/shaders/color.vsh.spv");
		psiColored.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/color.psh.spv");
		VulkanGpuProgram* programColored = reinterpret_cast<VulkanGpuProgram*>(_gpuProgramManager->getProgram(psiColored));

		std::vector<VkVertexInputAttributeDescription> vertexInputAttributs = std::move(ColoredVertex::getVertexAttributesDescription());

		VertexDescription vertexDescription;
		vertexDescription.bindings_strides.push_back(std::make_pair(0, sizeof(ColoredVertex)));
		vertexDescription.attributesCount = static_cast<uint32_t>(vertexInputAttributs.size());
		vertexDescription.attributes = vertexInputAttributs.data();

		VulkanPrimitiveTopology primitiveTopology = { LINE_LIST , false };
		VulkanRasterizationState rasterisation(CULL_MODE_NONE, POLYGON_MODE_FILL);
		VulkanDepthState depthState(true, true, VK_COMPARE_OP_LESS);
		VulkanStencilState stencilState(false);

		_commonPipelines[static_cast<uint8_t>(CommonPipelines::COMMON_PIPELINE_LINES)] = _renderer->getGraphicsPipeline(
			vertexDescription,
			primitiveTopology,
			rasterisation,
			CommonBlendModes::blend_none,
			depthState,
			stencilState,
			programColored
		);

		primitiveTopology.topology = TRIANGLE_LIST;
		_commonPipelines[static_cast<uint8_t>(CommonPipelines::COMMON_PIPELINE_COLORED_PLAINS)] = _renderer->getGraphicsPipeline(
			vertexDescription,
			primitiveTopology,
			rasterisation,
			CommonBlendModes::blend_none,
			depthState,
			stencilState,
			programColored
		);

		///////////// -> TexturedVertexes
		std::vector<engine::ProgramStageInfo> psiCTextured;
		psiCTextured.emplace_back(ProgramStage::VERTEX, "resources/shaders/texture.vsh.spv");
		psiCTextured.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/texture.psh.spv");
		VulkanGpuProgram* programTextured = reinterpret_cast<VulkanGpuProgram*>(_gpuProgramManager->getProgram(psiCTextured));

		vertexInputAttributs = std::move(TexturedVertex::getVertexAttributesDescription());
		vertexDescription.bindings_strides[0] = std::make_pair(0, sizeof(TexturedVertex));
		vertexDescription.attributesCount = static_cast<uint32_t>(vertexInputAttributs.size());
		vertexDescription.attributes = vertexInputAttributs.data();

		_commonPipelines[static_cast<uint8_t>(CommonPipelines::COMMON_PIPELINE_TEXTURED_DEPTH_RW)] = _renderer->getGraphicsPipeline(
			vertexDescription,
			primitiveTopology,
			rasterisation,
			CommonBlendModes::blend_alpha,
			depthState,
			stencilState,
			programTextured
		);

		depthState.depthWriteEnabled = false;
		depthState.depthTestEnabled = false;

		_commonPipelines[static_cast<uint8_t>(CommonPipelines::COMMON_PIPELINE_TEXTURED)] = _renderer->getGraphicsPipeline(
			vertexDescription,
			primitiveTopology,
			rasterisation,
			CommonBlendModes::blend_alpha,
			depthState,
			stencilState,
			programTextured
		);

		/////// test only - delete this code
		depthState.depthWriteEnabled = true;
		depthState.depthTestEnabled = true;

		std::vector<engine::ProgramStageInfo> psi_shadow_test;
		psi_shadow_test.emplace_back(ProgramStage::VERTEX, "resources/shaders/shadows_plain.vsh.spv");
		psi_shadow_test.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/shadows_plain.psh.spv");
		vulkan::VulkanGpuProgram* texture_shadow = _gpuProgramManager->getProgram(psi_shadow_test);

		_commonPipelines[static_cast<uint8_t>(CommonPipelines::TEST)] = _renderer->getGraphicsPipeline(
			vertexDescription,
			primitiveTopology,
			rasterisation,
			CommonBlendModes::blend_alpha,
			depthState,
			stencilState,
			texture_shadow
		);

		std::vector<engine::ProgramStageInfo> psi_shadow_test2;
		psi_shadow_test2.emplace_back(ProgramStage::VERTEX, "resources/shaders/texture.vsh.spv");
		psi_shadow_test2.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/texture_shadow.psh.spv");
		vulkan::VulkanGpuProgram* texture_shadow2 = _gpuProgramManager->getProgram(psi_shadow_test2);

		_commonPipelines[static_cast<uint8_t>(CommonPipelines::TEST2)] = _renderer->getGraphicsPipeline(
			vertexDescription,
			primitiveTopology,
			rasterisation,
			CommonBlendModes::blend_alpha,
			depthState,
			stencilState,
			texture_shadow2
		);
	}

	void RenderHelper::updateFrame() {
		const uint32_t currentFrame = _renderer->getCurrentFrame();

		_dynamic_vertices[currentFrame].recreate(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		_dynamic_indices[currentFrame].recreate(VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	}

	void RenderHelper::onResize() {}

	const vulkan::VulkanBuffer& RenderHelper::addDynamicVerteces(const void* data, const size_t sz, size_t& out_offset) {
		const uint32_t currentFrame = _renderer->getCurrentFrame();
		return _dynamic_vertices[currentFrame].addData(data, sz, out_offset);
	}

	const vulkan::VulkanBuffer& RenderHelper::addDynamicIndices(const void* data, const size_t sz, size_t& out_offset) {
		const uint32_t currentFrame = _renderer->getCurrentFrame();
		return _dynamic_indices[currentFrame].addData(data, sz, out_offset);
	}
}