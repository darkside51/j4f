#pragma once

#include "../Vulkan/vkGPUProgram.h"
#include "Engine/Core/Math/mathematic.h"

#include <cstdint>
#include <vector>
#include <memory>
#include <optional>

namespace engine {

	class SimpleInstanceStrategy {
	public:
		SimpleInstanceStrategy(const std::vector<mat4f>& transforms, const std::vector<vulkan::VulkanGpuProgram*>& programs) {
			for (auto&& p : programs) {
				p->setValueByName("models", transforms.data(), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, sizeof(mat4f) * transforms.size(), true);
			}
		}

		SimpleInstanceStrategy(std::vector<mat4f>&& transforms, const std::vector<vulkan::VulkanGpuProgram*>& programs) {
			for (auto&& p : programs) {
				p->setValueByName("models", transforms.data(), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, sizeof(mat4f) * transforms.size(), true);
			}
		}

		inline void updateGPUTransfromsData(const std::vector<vulkan::VulkanGpuProgram*>& programs) const {}
	};

	template <typename T, typename Strategy>
	class InstanceRenderer {
	public:
		template <typename... Args>
		InstanceRenderer(const std::vector<mat4f>& transforms, T* graphics, Args&&...args) :
			_instanceCount(transforms.size()), 
			_graphics(graphics),
			_strategy(std::make_unique<Strategy>(transforms, std::forward<Args>(args)...))
		{
			updateGraphicsInstanceCount();
		}

		template <typename... Args>
		InstanceRenderer(std::vector<mat4f>&& transforms, T* graphics, Args&&...args) :
			_instanceCount(transforms.size()), 
			_graphics(graphics),
			_strategy(std::make_unique<Strategy>(transforms, std::forward<Args>(args)...))
		{
			updateGraphicsInstanceCount();
		}

		~InstanceRenderer() {
			_graphics = nullptr;
			_strategy = nullptr;
		}

		inline void updateGPUTransfromsData(const std::vector<vulkan::VulkanGpuProgram*>& programs) const {
			_strategy->updateGPUTransfromsData(programs);
		}

		inline void setGraphics(T* g) {
			_graphics = std::unique_ptr<T>(g);
			updateGraphicsInstanceCount();
		}

		inline void setGraphics(std::unique_ptr<T>&& g) {
			_graphics = std::move(g);
			updateGraphicsInstanceCount();
		}

		inline void updateRenderData(const mat4f& worldMatrix, const bool worldMatrixChanged) {
			_graphics->updateRenderData(worldMatrix, worldMatrixChanged);
		}

		inline void updateModelMatrixChanged(const bool worldMatrixChanged) {
			_graphics->updateModelMatrixChanged(worldMatrixChanged);
		}

		inline vulkan::VulkanGpuProgram* setProgram(vulkan::VulkanGpuProgram* program, VkRenderPass renderPass = nullptr) {
			return _graphics->setProgram(program, renderPass);
		}

		inline const RenderDescriptor& getRenderDescriptor() const { return _graphics->getRenderDescriptor(); }
		inline RenderDescriptor& getRenderDescriptor() { return _graphics->getRenderDescriptor(); }

	private:

		inline void updateGraphicsInstanceCount() {
			if (_graphics) {
				auto&& rdescriptor = _graphics->getRenderDescriptor();
				for (size_t i = 0; i < rdescriptor.renderDataCount; ++i) {
					for (size_t j = 0; j < rdescriptor.renderData[i]->renderPartsCount; ++j) {
						rdescriptor.renderData[i]->renderParts[j].instanceCount = _instanceCount;
					}
				}
			}
		}

		uint32_t _instanceCount;
		std::unique_ptr<T> _graphics;
		std::unique_ptr<Strategy> _strategy;
	};

}