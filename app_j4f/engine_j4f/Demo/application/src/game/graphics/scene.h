#pragma once

#include <Engine/Core/Common.h>
#include <Engine/Core/Hierarchy.h>
#include <Engine/Core/Ref_ptr.h>
#include <Engine/Graphics/Camera.h>
#include <Engine/Graphics/Scene/NodeGraphicsLink.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace engine {
	class Node;
	class NodeRenderer;
	class GraphicsDataUpdater;
	class ImguiStatObserver;
	class ImguiGraphics;
}

namespace game {
	template <typename T>
	using NodeRenderer = engine::NodeRendererImpl<T>;

	using NodeHR = engine::HierarchyRaw<engine::Node>;
	using NodePtr = engine::ref_ptr<NodeHR>;

	class Scene : public engine::ICameraTransformChangeObserver {
	public:
		Scene();
		~Scene();

		void update(const float delta);
		void render(const float delta);
		void resize(const uint16_t w, const uint16_t h);

		engine::ref_ptr<engine::Camera>& getWorldCamera() noexcept {
			return _worldCamera;
		}

		template <typename T>
		NodePtr placeToWorld(NodeRenderer<T>* graphics) {
			return placeToWorld(graphics, engine::UniqueTypeId<Scene>::getUniqueId<T>());
		}

		template <typename T>
		NodePtr placeToUi(NodeRenderer<T>* graphics) {
			return placeToUi(graphics, engine::UniqueTypeId<Scene>::getUniqueId<T>());
		}

		engine::ref_ptr<engine::ImguiGraphics>& getUiGraphics() noexcept { return _imguiGraphics; }

	private:
		NodePtr placeToWorld(engine::NodeRenderer* graphics, uint16_t typeId);
		NodePtr placeToUi(engine::NodeRenderer* graphics, uint16_t typeId);

		void onCameraTransformChanged(const engine::Camera* camera) override;
		void registerGraphicsUpdateSystems();

		std::unique_ptr<engine::GraphicsDataUpdater> _graphicsDataUpdater = nullptr;

		std::unique_ptr<engine::ImguiStatObserver> _statObserver = nullptr;
		engine::ref_ptr<engine::ImguiGraphics> _imguiGraphics = nullptr;

		std::unique_ptr<NodeHR> _rootNode;
		std::unique_ptr<NodeHR> _uiNode;

		std::vector<std::unique_ptr<engine::Camera>> _cameras;
		engine::ref_ptr<engine::Camera> _worldCamera;
	};
}