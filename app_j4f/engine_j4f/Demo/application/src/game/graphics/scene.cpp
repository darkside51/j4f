#include "scene.h"

#include "camera_controller.h"

#include <Engine/Core/Common.h>
#include <Engine/Core/Math/mathematic.h>

#include <Engine/Graphics/Animation/AnimationManager.h>
#include <Engine/Graphics/Animation/ActionAnimation.h>
#include <Engine/Graphics/Mesh/AnimationTree.h>
#include <Engine/Graphics/Plane/Plane.h>
#include <Engine/Graphics/Render/RenderList.h>
#include <Engine/Graphics/Scene/Node.h>
#include <Engine/Graphics/Scene/NodeGraphicsLink.h>
#include <Engine/Graphics/Scene/NodeRenderListHelper.h>
#include <Engine/Graphics/UI/ImGui/Imgui.h>
#include <Engine/Utils/ImguiStatObserver.h>

#include <imgui.h>

#include <cstdint>

namespace game {
    constexpr uint16_t kUiOrder = 255u;
    engine::RenderList rootRenderList;
    engine::RenderList uiRenderList;

    Scene::Scene() :
            _graphicsDataUpdater(std::make_unique<engine::GraphicsDataUpdater>()),
            _statObserver(std::make_unique<engine::ImguiStatObserver>(engine::ImguiStatObserver::Location::top_left)),
            _rootNode(std::make_unique<NodeHR>()),
            _uiNode(std::make_unique<NodeHR>()) {
        registerGraphicsUpdateSystems();

        using namespace engine;

        auto &&renderer = Engine::getInstance().getModule<Graphics>().getRenderer();
        const auto [width, height] = renderer->getSize();
        auto &worldCamera = _cameras.emplace_back(width, height);
        worldCamera.addObserver(this);
        worldCamera.enableFrustum();
        worldCamera.makeProjection(math_constants::f32::pi / 3.0f,
                                   static_cast<float>(width) / static_cast<float>(height), 1.0f, 5000.0f);

        auto imgui = std::make_unique<NodeRenderer<ImguiGraphics *>>();
        imgui->setGraphics(ImguiGraphics::getInstance());
        imgui->getRenderDescriptor()->order = kUiOrder;
        _imguiGraphics = imgui->graphics();
        {
            placeToUi(imgui.release());
        }
    }

    Scene::~Scene() {
        _rootNode = nullptr;
        _uiNode = nullptr;
    }

    void Scene::onCameraTransformChanged(const engine::Camera * /*camera*/) {}

    void Scene::registerGraphicsUpdateSystems() {
        using namespace engine;
        registerUpdateSystem<ImguiGraphics *>();
        registerUpdateSystem<Mesh *>();
        registerUpdateSystem<Plane *>();
    }

    void Scene::resize(const uint16_t w, const uint16_t h) {
        _cameras[0].resize(w, h);
    }

    void Scene::assignCameraController(engine::ref_ptr<CameraController> controller) noexcept {
        _controller = controller;
        _controller->assignCamera(engine::make_ref(_cameras[0]));
    }

    void Scene::update(const float delta) {
        using namespace engine;
        Engine::getInstance().getModule<Graphics>().getAnimationManager()->update(delta);
        //Engine::getInstance().getModule<Graphics>().getAnimationManager()->update<MeshAnimationTree, ActionAnimation>(delta);
    }

    void Scene::render(const float delta) {
        using namespace engine;

        auto &worldCamera = _cameras[0];

        auto &&renderer = Engine::getInstance().getModule<Graphics>().getRenderer();
        const uint32_t currentFrame = renderer->getCurrentFrame();

        const auto [width, height] = renderer->getSize();

        auto &commandBuffer = renderer->getRenderCommandBuffer();
        commandBuffer.begin();

        std::array<VkClearValue, 2> clearValues = {
                VkClearValue{0.5f, 0.5f, 0.5f, 1.0f}, // for 1st attachment (color)
                VkClearValue{1.0f, 0} // for 2nd attachment (depthStencil)
        };
        commandBuffer.cmdBeginRenderPass(renderer->getMainRenderPass(), {{0,     0},
                                                                         {width, height}},
                                         clearValues.data(), clearValues.size(),
                                         renderer->getFrameBuffer().m_framebuffer, VK_SUBPASS_CONTENTS_INLINE);

        commandBuffer.cmdSetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f,
                                     false);
        commandBuffer.cmdSetScissor(0, 0, width, height);
        commandBuffer.cmdSetDepthBias(0.0f, 0.0f, 0.0f);

        { // fill rootNode
//            const bool mainCameraDirty = worldCamera.calculateTransform();
            const bool mainCameraDirty = _controller ? _controller->update(delta) : false;
            reloadRenderList(rootRenderList, _rootNode.get(), mainCameraDirty, 0u,
                             engine::FrustumVisibleChecker(worldCamera.getFrustum()));
        }

        { // fill uiNode
            reloadRenderList(uiRenderList, _uiNode.get(), false, 0u);

            if (_imguiGraphics) {
                _imguiGraphics->update(delta);
                _statObserver->draw();

                //ImGui::ShowDemoWindow();
            }
        }

        _graphicsDataUpdater->updateData();
        //_graphicsDataUpdater->updateData<GraphicsDataUpdateSystem<Mesh*>>();

        // render nodes
        rootRenderList.render(commandBuffer, currentFrame, {&worldCamera.getTransform(), nullptr, nullptr});
        uiRenderList.render(commandBuffer, currentFrame, {nullptr, nullptr, nullptr});

        commandBuffer.cmdEndRenderPass();
        commandBuffer.end();
    }
}