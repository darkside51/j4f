#pragma once

#include <Engine/Core/Common.h>
#include <Engine/Core/ref_ptr.h>
#include <Engine/Graphics/Camera.h>
#include <Engine/Utils/Debug/Assert.h>

#include "definitions.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace engine {
    class CascadeShadowMap;
	class ImguiStatObserver;
	class ImguiGraphics;
}

namespace game {
    class CameraController;
    class UIManager;

	class Scene : public engine::ICameraTransformChangeObserver {
	public:
		Scene();
		~Scene();

		void update(const float delta);
		void render(const float delta);
		void resize(const uint16_t w, const uint16_t h);

		engine::Camera& getCamera(uint8_t id) noexcept {
			return _cameras[id];
		}

        void assignCameraController(engine::ref_ptr<CameraController> controller) noexcept;

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

        void addShadowCastNode(NodePtr node) {
            _shadowCastNodes.emplace_back(node);
        }

        void removeShadowCastNode(NodePtr node) {
            _shadowCastNodes.erase(
                    std::remove(_shadowCastNodes.begin(), _shadowCastNodes.end(), node),
                    _shadowCastNodes.end());
        }

		engine::ref_ptr<engine::ImguiGraphics>& getUiGraphics() noexcept { return _imguiGraphics; }
        engine::ref_ptr<engine::CascadeShadowMap> getShadowMap() noexcept { return _shadowMap; }
        engine::ref_ptr<UIManager> getUIManager() noexcept { return _uiManager; }

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

        std::unique_ptr<UIManager> _uiManager;

        std::unique_ptr<engine::CascadeShadowMap> _shadowMap;
        std::vector<NodePtr> _shadowCastNodes;

		std::vector<engine::Camera> _cameras;
        engine::ref_ptr<CameraController> _controller;
	};
}