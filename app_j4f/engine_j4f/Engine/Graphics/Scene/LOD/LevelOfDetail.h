#pragma once

#include "../NodeGraphicsLink.h"
#include <cstdint>
#include <memory>

namespace vulkan {
	class VulkanGpuProgram;
}

namespace engine {

	class Camera;
	class Node;

	struct ILodIndex {
		virtual ~ILodIndex() = default;
	};

	template <typename Strategy>
	struct LodIndex : public ILodIndex {
		inline uint8_t getLod(Node* node, Camera* camera) const {
			return Strategy::getLod(node, camera);
		}
	};

	template <typename T> requires IsRenderAvailableType<T>
	class LevelOfDetail {
	public:
		using type = std::unique_ptr<T>;

		~LevelOfDetail() {
			_graphics = nullptr;
		}

		inline void applyTo(NodeRendererImpl<T>* NodeRendererImpl) {}

	private:
		type _graphics = nullptr;
		vulkan::VulkanGpuProgram* _program;
	};
}