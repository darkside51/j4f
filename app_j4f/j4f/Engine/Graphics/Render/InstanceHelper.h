#pragma once

#include "../Vulkan/vkGPUProgram.h"
#include "../../Core/Math/math.h"

#include <cstdint>
#include <vector>
#include <memory>
#include <optional>

namespace engine {

	template <typename T>
	class InstanceRenderer {
	public:
		InstanceRenderer(const std::vector<vulkan::VulkanGpuProgram*>& programs, const std::vector<glm::mat4>& transforms, const bool storeTransforms) : _instanceCount(transforms.size()), _graphics(nullptr) {
			for (auto&& p : programs) {
				p->setValueByName("models", transforms.data(), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, sizeof(glm::mat4) * _instanceCount, true);
			}

			if (storeTransforms) {
				_transforms = transforms;
			}
		}

		InstanceRenderer(const std::vector<vulkan::VulkanGpuProgram*>& programs, std::vector<glm::mat4>&& transforms, const bool storeTransforms) : _instanceCount(transforms.size()), _graphics(nullptr) {
			for (auto&& p : programs) {
				p->setValueByName("models", transforms.data(), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, sizeof(glm::mat4) * _instanceCount, true);
			}

			if (storeTransforms) {
				_transforms = std::move(transforms);
			}
		}

		~InstanceRenderer() {
			_graphics = nullptr;
		}

		inline void updateGPUTransfromsData(const std::vector<vulkan::VulkanGpuProgram*>& programs) const {
			if (_transforms.has_value()) {
				for (auto&& p : programs) {
					p->setValueByName("models", _transforms.value().data(), nullptr, vulkan::VulkanGpuProgram::UNDEFINED, sizeof(glm::mat4) * _instanceCount, true);
				}
			}
		}

		inline void setGraphics(T* g) {
			_graphics = std::unique_ptr<T>(g);

			auto&& rdescriptor = _graphics->getRenderDescriptor();
			for (size_t i = 0; i < rdescriptor.renderDataCount; ++i) {
				for (size_t j = 0; j < rdescriptor.renderData[i]->renderPartsCount; ++j) {
					rdescriptor.renderData[i]->renderParts[j].instanceCount = _instanceCount;
				}
			}
		}

		inline void setGraphics(std::unique_ptr<T>&& g) {
			_graphics = std::move(g);

			auto&& rdescriptor = _graphics->getRenderDescriptor();
			for (size_t i = 0; i < rdescriptor.renderDataCount; ++i) {
				for (size_t j = 0; j < rdescriptor.renderData[i]->renderPartsCount; ++j) {
					rdescriptor.renderData[i]->renderParts[j].instanceCount = _instanceCount;
				}
			}
		}

		inline void updateRenderData(const glm::mat4& worldMatrix, const bool worldMatrixChanged) {
			_graphics->updateRenderData(worldMatrix, worldMatrixChanged);
		}

		inline vulkan::VulkanGpuProgram* setProgram(vulkan::VulkanGpuProgram* program, VkRenderPass renderPass = nullptr) {
			return _graphics->setProgram(program, renderPass);
		}

		inline const RenderDescriptor& getRenderDescriptor() const { return _graphics->getRenderDescriptor(); }
		inline RenderDescriptor& getRenderDescriptor() { return _graphics->getRenderDescriptor(); }

	private:
		uint32_t _instanceCount;
		std::unique_ptr<T> _graphics;
		std::optional<std::vector<glm::mat4>> _transforms;
	};

}