#include "RenderHelper.h"

#include "../../Core/Engine.h"
#include "../Graphics.h"
#include "../GpuProgramsManager.h"
#include "../Vulkan/vkRenderer.h"
#include "../Vulkan/vkHelper.h"
#include "AutoBatchRender.h"

#include "../../Core/Math/mathematic.h"

namespace engine {

	void VulkanStreamBuffer::createBuffer(const size_t sz) {
		constexpr size_t minBufferSize = 1024 * 100; // 100kb
		size = std::max(sz, minBufferSize);

		auto&& renderer = Engine::getInstance().getModule<Graphics>().getRenderer();
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
		const uint8_t swapChainImagesCount = _renderer->getSwapchainImagesCount();
		_dynamic_vertices.resize(swapChainImagesCount);
		_dynamic_indices.resize(swapChainImagesCount);
	}

	RenderHelper::~RenderHelper() {
		_dynamic_vertices.clear();
		_dynamic_indices.clear();
		delete _autoBatchRenderer;

		if (_debugDrawRenderData) {
			delete _debugDrawRenderData;
			_debugDrawRenderData = nullptr;
		}
	}

	void RenderHelper::initCommonPipelines() {
		using namespace vulkan;

		std::vector<engine::ProgramStageInfo> psiColored;
		psiColored.emplace_back(ProgramStage::VERTEX, "resources/shaders/color.vsh.spv");
		psiColored.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/color.psh.spv");
		VulkanGpuProgram* programColored = reinterpret_cast<VulkanGpuProgram*>(_gpuProgramManager->getProgram(psiColored));

		VertexDescription vertexDescription;
		vertexDescription.bindings_strides.push_back(std::make_pair(0, sizeof(ColoredVertex)));
		vertexDescription.attributes = ColoredVertex::getVertexAttributesDescription();

		VulkanPrimitiveTopology primitiveTopology = { PrimitiveTopology::LINE_LIST , false };
		VulkanRasterizationState rasterisation(CullMode::CULL_MODE_NONE, PoligonMode::POLYGON_MODE_FILL);
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

		primitiveTopology.topology = PrimitiveTopology::TRIANGLE_LIST;
		_commonPipelines[static_cast<uint8_t>(CommonPipelines::COMMON_PIPELINE_COLORED_PLANES)] = _renderer->getGraphicsPipeline(
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

		vertexDescription.bindings_strides[0] = std::make_pair(0, sizeof(TexturedVertex));
		vertexDescription.attributes = TexturedVertex::getVertexAttributesDescription();

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

		_debugDrawRenderData = new vulkan::RenderData(const_cast<vulkan::VulkanPipeline*>(getPipeline(CommonPipelines::COMMON_PIPELINE_LINES)));
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

	/////
	void RenderHelper::drawBoundingBox(const vec3f& c1, const vec3f& c2, const mat4f& cameraMatrix, const mat4f& worldMatrix, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const bool batch) {
		
		constexpr uint32_t vertexBufferSize = 8 * sizeof(ColoredVertex);
		constexpr uint32_t indexBufferSize = 24 * sizeof(uint32_t);
		
		constexpr uint32_t idxs[24] = {
				0, 1,
				0, 2,
				2, 3,
				3, 1,

				4, 5,
				4, 6,
				6, 7,
				7, 5,

				0, 4,
				1, 5,
				2, 6,
				3, 7
		};
	
		if (batch) {
			_debugDrawRenderData->setParamByName("mvp", &const_cast<mat4f&>(cameraMatrix), true);

			const vec4f vtxCoords[8] = {
				worldMatrix * vec4f(c1.x, c1.y, c1.z, 1.0f),
				worldMatrix * vec4f(c1.x, c2.y, c1.z, 1.0f),
				worldMatrix * vec4f(c2.x, c1.y, c1.z, 1.0f),
				worldMatrix * vec4f(c2.x, c2.y, c1.z, 1.0f),

				worldMatrix * vec4f(c1.x, c1.y, c2.z, 1.0f),
				worldMatrix * vec4f(c1.x, c2.y, c2.z, 1.0f),
				worldMatrix * vec4f(c2.x, c1.y, c2.z, 1.0f),
				worldMatrix * vec4f(c2.x, c2.y, c2.z, 1.0f),
			};

            const ColoredVertex vtx[8] = {
                    { {vtxCoords[0].x, vtxCoords[0].y, vtxCoords[0].z}, 0xff000000},
                    { {vtxCoords[1].x, vtxCoords[1].y, vtxCoords[1].z}, 0xff00ff00 },
                    { {vtxCoords[2].x, vtxCoords[2].y, vtxCoords[2].z}, 0xff0000ff },
                    { {vtxCoords[3].x, vtxCoords[3].y, vtxCoords[3].z}, 0xff00ffff },

                    { {vtxCoords[4].x, vtxCoords[4].y, vtxCoords[4].z}, 0xffff0000 },
                    { {vtxCoords[5].x, vtxCoords[5].y, vtxCoords[5].z}, 0xffffff00 },
                    { {vtxCoords[6].x, vtxCoords[6].y, vtxCoords[6].z}, 0xffff00ff },
                    { {vtxCoords[7].x, vtxCoords[7].y, vtxCoords[7].z}, 0xffffffff }
            };

			_autoBatchRenderer->addToDraw(_debugDrawRenderData, sizeof(ColoredVertex), &vtx[0], vertexBufferSize, &idxs[0], indexBufferSize, commandBuffer, currentFrame);
		} else {
            const ColoredVertex vtx[8] = {
                    { {c1.x, c1.y, c1.z}, 0xff000000 },
                    { {c1.x, c2.y, c1.z}, 0xff00ff00 },
                    { {c2.x, c1.y, c1.z}, 0xff0000ff },
                    { {c2.x, c2.y, c1.z}, 0xff00ffff },

                    { {c1.x, c1.y, c2.z}, 0xffff0000 },
                    { {c1.x, c2.y, c2.z}, 0xffffff00 },
                    { {c2.x, c1.y, c2.z}, 0xffff00ff },
                    { {c2.x, c2.y, c2.z}, 0xffffffff }
            };

			const mat4f transform = cameraMatrix * worldMatrix;

			size_t vOffset;
			size_t iOffset;

			auto&& vBuffer = addDynamicVerteces(&vtx[0], vertexBufferSize, vOffset);
			auto&& iBuffer = addDynamicIndices(&idxs[0], indexBufferSize, iOffset);

			static auto&& pipeline = getPipeline(CommonPipelines::COMMON_PIPELINE_LINES);
			static const vulkan::GPUParamLayoutInfo* mvp_layout = pipeline->program->getGPUParamLayoutByName("mvp");

			vulkan::VulkanPushConstant pushConstats;
			const_cast<vulkan::VulkanGpuProgram*>(pipeline->program)->setValueToLayout(mvp_layout, &transform, &pushConstats);
			//commandBuffer.renderIndexed(pipeline, currentFrame, &pushConstats, 0, 0, nullptr, 0, nullptr, vBuffer, iBuffer, 0, 24, 0, 1, 0, vOffset, iOffset);
			commandBuffer.renderIndexed(pipeline, currentFrame, &pushConstats, 0, 0, nullptr, 0, nullptr, vBuffer, iBuffer, iOffset / sizeof(uint32_t), 24, 0, 1, 0, vOffset, 0); // �� ����� ������������ ���������� �������
		}
	}

	void RenderHelper::drawSphere(const vec3f& c, const float r, const mat4f& cameraMatrix, const mat4f& worldMatrix, vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const bool batch) {
		constexpr uint8_t segments = 64;
		constexpr float a = math_constants::f32::pi2 / segments;
		
		ColoredVertex vtx[3 * segments];
		uint32_t idxs[3 * segments * 2];

		const vec4f center = worldMatrix * vec4f(c.x, c.y, c.z, 1.0f);
		const float radius = glm::length(worldMatrix[0]) * r;

		uint8_t vtxCount = 0;
		float angle = 0.0f;
		for (uint8_t i = 0; i < segments; ++i) {
			const float x = center.x + radius * cosf(angle);
			const float y = center.y + radius * sinf(angle);
			const float z = center.z;

			const uint8_t j = vtxCount + i;

            vtx[j] = { {x, y, z}, 0xffff0000 };

			idxs[2 * j] = j;
			idxs[2 * j + 1] = i == (segments - 1) ? vtxCount : j + 1;

			angle += a;
		}

		vtxCount += segments;
		angle = 0.0f;
		for (uint8_t i = 0; i < segments; ++i) {
			const float x = center.x + radius * cosf(angle);
			const float y = center.y;
			const float z = center.z + radius * sinf(angle);

			const uint8_t j = vtxCount + i;

            vtx[j] = { {x, y, z}, 0xff00ff00 };

			idxs[2 * j] = j;
			idxs[2 * j + 1] = i == (segments - 1) ? vtxCount : j + 1;

			angle += a;
		}

		vtxCount += segments;
		angle = 0.0f;
		for (uint8_t i = 0; i < segments; ++i) {
			const float x = center.x;
			const float y = center.y + radius * sinf(angle);
			const float z = center.z + radius * cosf(angle);

			const uint8_t j = vtxCount + i;

            vtx[j] = { {x, y, z}, 0xff0000ff };

			idxs[2 * j] = j;
			idxs[2 * j + 1] = i == (segments - 1) ? vtxCount : j + 1;

			angle += a;
		}

		_debugDrawRenderData->setParamByName("mvp", &const_cast<mat4f&>(cameraMatrix), true);

		constexpr uint32_t vertexBufferSize = 3 * segments * sizeof(ColoredVertex);
		constexpr uint32_t indexBufferSize = 3 * 2 * segments * sizeof(uint32_t);

		_autoBatchRenderer->addToDraw(_debugDrawRenderData, sizeof(ColoredVertex), &vtx[0], vertexBufferSize, &idxs[0], indexBufferSize, commandBuffer, currentFrame);
	}
}