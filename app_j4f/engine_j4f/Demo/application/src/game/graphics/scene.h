#pragma once

#include <Engine/Core/Common.h>
#include <Engine/Core/Hierarchy.h>
#include <Engine/Core/Ref_ptr.h>
#include <Engine/Graphics/Camera.h>
#include <Engine/Graphics/Scene/NodeGraphicsLink.h>
#include <Engine/Utils/Debug/Assert.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace engine {
	class Node;
	class NodeRenderer;
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

		engine::Camera& getWorldCamera() noexcept {
			return _cameras[0];
		}

        NodePtr placeToWorld() {
            auto* node = new NodeHR();
            _rootNode->addChild(node);
            return NodePtr(node);
        }

        NodePtr placeToUi() {
            auto* node = new NodeHR();
            _uiNode->addChild(node);
            return NodePtr(node);
        }

		template <typename T>
		NodePtr placeToWorld(NodeRenderer<T>* graphics) {
            return placeToNode(graphics, NodePtr(_rootNode.get()));
		}

		template <typename T>
		NodePtr placeToUi(NodeRenderer<T>* graphics) {
            return placeToNode(graphics, NodePtr(_uiNode.get()));
		}

        template <typename T>
        void registerGraphisObject(NodeRenderer<T>* graphics) {
            auto const typeId = engine::UniqueTypeId<Scene>::getUniqueId<T>();
            if (typeId < _graphicsUpdateSystems.size()) {
                auto && system = static_cast<engine::GraphicsDataUpdateSystem<T>*>(_graphicsUpdateSystems[typeId].get());
                system->registerObject(graphics);
            }
        }

        template <typename T>
        NodePtr placeToNode(NodeRenderer<T>* graphics, NodePtr parent) {
            auto* node = new NodeHR(graphics);
            parent->addChild(node);
            registerGraphisObject(graphics);
            return NodePtr(node);
        }

		engine::ref_ptr<engine::ImguiGraphics>& getUiGraphics() noexcept { return _imguiGraphics; }

	private:
        template <typename T>
        void registerUpdateSystem() {
            auto const typeId = engine::UniqueTypeId<Scene>::getUniqueId<T>();
            auto system = std::make_unique<engine::GraphicsDataUpdateSystem<T>>();
            _graphicsDataUpdater->registerSystem(system.get());
            auto const systemsCount = _graphicsUpdateSystems.size();
            if (typeId < systemsCount) {
                _graphicsUpdateSystems[typeId] = std::move(system);
            } else {
                ENGINE_BREAK_CONDITION(typeId == systemsCount);
                _graphicsUpdateSystems.emplace_back(std::move(system));
            }
        }

		void onCameraTransformChanged(const engine::Camera* camera) override;
		void registerGraphicsUpdateSystems();

		std::unique_ptr<engine::GraphicsDataUpdater> _graphicsDataUpdater;
        std::vector<std::unique_ptr<engine::IGraphicsDataUpdateSystem>> _graphicsUpdateSystems;

		std::unique_ptr<engine::ImguiStatObserver> _statObserver;
		engine::ref_ptr<engine::ImguiGraphics> _imguiGraphics;

		std::unique_ptr<NodeHR> _rootNode;
		std::unique_ptr<NodeHR> _uiNode;

		std::vector<engine::Camera> _cameras;
	};
}