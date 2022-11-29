#pragma once

#include "../Vulkan/vkRenderData.h"
#include "RenderDescriptor.h"

#include <string>
#include <utility>

namespace engine {

	class RenderedEntity {
	public:
		virtual ~RenderedEntity() {
			_renderDescriptor.destroy();
		}

		template <typename T>
		inline void setParamByName(const std::string& name, T* value, bool copyData, const uint32_t count = 1) {
			for (uint32_t i = 0; i < _renderDescriptor.renderDataCount; ++i) {
				_renderDescriptor.renderData[i]->setParamByName(name, value, copyData, count);
			}
		}

		template <typename T>
		inline void setParamForLayout(const vulkan::GPUParamLayoutInfo* info, T* value, const bool copyData, const uint32_t count = 1) {
			for (uint32_t i = 0; i < _renderDescriptor.renderDataCount; ++i) {
				_renderDescriptor.renderData[i]->setParamForLayout(info, value, copyData, count);
			}
		}

		inline vulkan::RenderData* getRenderDataAt(const uint16_t n) const {
			if (_renderDescriptor.renderDataCount <= n) return nullptr;
			return _renderDescriptor.renderData[n];
		}

		inline vulkan::VulkanRenderState& renderState() { return _renderState; }
		inline const vulkan::VulkanRenderState& renderState() const { return _renderState; }

		inline const RenderDescriptor& getRenderDescriptor() const { return _renderDescriptor; }
		inline RenderDescriptor& getRenderDescriptor() { return _renderDescriptor; }

		void setPipeline(vulkan::VulkanPipeline* p) {
			if (_renderDescriptor.renderData[0]->pipeline == nullptr || _renderDescriptor.renderData[0]->pipeline->program != p->program) {
				for (auto&& l : _fixedGpuLayouts) {
					l.first = p->program->getGPUParamLayoutByName(l.second);
				}
			}

			_renderDescriptor.setCameraMatrix(_fixedGpuLayouts[0].first);
			for (uint32_t i = 0; i < _renderDescriptor.renderDataCount; ++i) {
				_renderDescriptor.renderData[i]->setPipeline(p);
			}
		}

		inline vulkan::VulkanGpuProgram* setProgram(vulkan::VulkanGpuProgram* program, VkRenderPass renderPass = nullptr) {
			if (_renderDescriptor.renderDataCount == 0) return nullptr;
			if (_vertexInputAttributes.empty()) return nullptr;

			auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();
			vulkan::VulkanPipeline* pipeline = renderer->getGraphicsPipeline(_renderState, program, renderPass);
			vulkan::VulkanPipeline* currentPipeline = _renderDescriptor.renderData[0]->pipeline;

			const vulkan::VulkanGpuProgram* currentProgram = currentPipeline ? currentPipeline->program : nullptr;

			if (currentPipeline == pipeline) return const_cast<vulkan::VulkanGpuProgram*>(currentProgram);

			setPipeline(pipeline);
			return const_cast<vulkan::VulkanGpuProgram*>(currentProgram);
		}

		inline void pipelineAttributesChanged() {
			setPipeline(Engine::getInstance().getModule<Graphics>()->getRenderer()->getGraphicsPipeline(_renderState, _renderDescriptor.renderData[0]->pipeline->program));
		}

		inline void render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const glm::mat4* cameraMatrix) {
			_renderDescriptor.render(commandBuffer, currentFrame, cameraMatrix);
		}

	protected:
		RenderDescriptor _renderDescriptor;
		vulkan::VulkanRenderState _renderState;
		std::vector<VkVertexInputAttributeDescription> _vertexInputAttributes;
		std::vector<std::pair<const vulkan::GPUParamLayoutInfo*, std::string>> _fixedGpuLayouts;
	};

}