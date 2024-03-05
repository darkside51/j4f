#include "scene.h"

#include "camera_controller.h"
#include "ui_manager.h"

#include <Engine/Core/Common.h>
#include <Engine/Core/Math/mathematic.h>

#include <Engine/Graphics/Animation/AnimationManager.h>
#include <Engine/Graphics/Animation/ActionAnimation.h>
#include <Engine/Graphics/Mesh/AnimationTree.h>
#include <Engine/Graphics/Plane/Plane.h>
#include <Engine/Graphics/Render/RenderList.h>
#include <Engine/Graphics/Render/RenderDescriptor.h>
#include <Engine/Graphics/Render/AutoBatchRender.h>
#include <Engine/Graphics/Scene/Node.h>
#include <Engine/Graphics/Scene/NodeGraphicsLink.h>
#include <Engine/Graphics/Scene/NodeRenderListHelper.h>
#include <Engine/Graphics/UI/ImGui/Imgui.h>
#include <Engine/Utils/ImguiStatObserver.h>

#include <Engine/Graphics/Features/Shadows/CascadeShadowMap.h>
#include <Engine/Graphics/Features/Shadows/ShadowMapHelper.h>

#include <cstdint>

namespace game {
    constexpr uint16_t kUiOrder = 255u;
    engine::RenderList rootRenderList;
    engine::RenderList uiRenderList;
    engine::RenderList shadowRenderList;

    //// cascade shadow map
    constexpr uint8_t kShadowMapCascadeCount = 3u;
    constexpr uint16_t kShadowMapDim = 2048u;
    const auto lightPos = engine::vec3f{-600.0f, -700.0f, 700.0f};
    //// cascade shadow map

    Scene::Scene() :
            _graphicsDataUpdater(std::make_unique<engine::GraphicsDataUpdater>()),
            _statObserver(std::make_unique<engine::ImguiStatObserver>(engine::ImguiStatObserver::Location::top_left)),
            _rootNode(std::make_unique<NodeHR>()),
            _uiNode(std::make_unique<NodeHR>()),
            _uiManager(std::make_unique<UIManager>()) {
        registerGraphicsUpdateSystems();

        using namespace engine;

        auto &&renderer = Engine::getInstance().getModule<Graphics>().getRenderer();
        const auto [width, height] = renderer->getSize();
        auto &worldCamera = _cameras.emplace_back(width, height);
        worldCamera.addObserver(this);
        worldCamera.enableFrustum();
        worldCamera.makeProjection(math_constants::f32::pi / 3.0f,
                                   static_cast<float>(width) / static_cast<float>(height), 1.0f, 3000.0f);

        auto imgui = std::make_unique<NodeRenderer<ImguiGraphics *>>();
        imgui->setGraphics(ImguiGraphics::getInstance("resources/assets/fonts/OpenSans/OpenSans-Bold.ttf", 20.0f));
        imgui->getRenderDescriptor().order = kUiOrder;
        _imguiGraphics = imgui->graphics();
        {
            placeToUi(imgui.release());
        }

        _shadowMap = std::make_unique<CascadeShadowMap>(kShadowMapDim, 32u, kShadowMapCascadeCount,
                                                        worldCamera.getNearFar(), 400.0f, 1200.0f);
        _shadowMap->setLamdas(1.0f, 1.0f, 1.0f);
        _shadowMap->setLightPosition(lightPos);

        auto mesh_skin_shadow_program = CascadeShadowMap::getShadowProgram<MeshSkinnedShadow>();
        _shadowMap->registerProgramAsReciever(mesh_skin_shadow_program);
    }

    Scene::~Scene() {
        _rootNode = nullptr;
        _uiNode = nullptr;
    }

    void Scene::onCameraTransformChanged(const engine::Camera* camera) {
        _shadowMap->updateCascades(camera);
    }

    void Scene::registerGraphicsUpdateSystems() {
        using namespace engine;
        registerUpdateSystem<ImguiGraphics*>();
        registerUpdateSystem<Mesh*>();
        registerUpdateSystem<Plane*>();
        registerUpdateSystem<ReferenceEntity<Mesh>*>();
    }

    void Scene::resize(const uint16_t w, const uint16_t h) {
        _controller->resize(w,h);
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

        auto &mainCamera = _cameras[0];

        { //
            constexpr float radius = 700.0f;
            static float angle = 1.0f;
            static bool night = false;
            angle += delta * 0.01f;
            const float c = cosf(angle);
            const float s = sinf(angle);
            const float h = (s > 0.5f) ? 1.0f : 0.0f;
            const vec3f lightPos(h * radius * c, 300.0f, h * radius * s);
            if (_shadowMap->setLightPosition(lightPos)) {
                night = false;
                _shadowMap->updateCascades(&mainCamera);
            } else if (!night) {
                night = true;
                angle += math_constants::f32::pi;
            }
        }

        auto && graphics = Engine::getInstance().getModule<Graphics>();
        auto && renderer = graphics.getRenderer();
        auto && renderHelper = graphics.getRenderHelper();
        auto && autoBatcher = renderHelper->getAutoBatchRenderer();

        const uint32_t currentFrame = renderer->getCurrentFrame();

        const auto [width, height] = renderer->getSize();

        { // fill rootNode
            const bool mainCameraDirty = _controller && _controller->update(delta);
            _shadowMap->updateShadowUniformsForRegesteredPrograms(mainCamera.getViewTransform());
            reloadRenderList(rootRenderList, _rootNode.get(), mainCameraDirty, 0u,
                             engine::FrustumVisibleChecker(mainCamera.getFrustum()), true);

            // shadows
            reloadRenderList(shadowRenderList, _shadowCastNodes.data(), _shadowCastNodes.size(),
                             false, 0u, {}, false);
        }

        { // fill uiNode
            reloadRenderList(uiRenderList, _uiNode.get(), false, 0u);
        }

        { // ui
            if (_imguiGraphics) {
                _imguiGraphics->update(delta);
                _uiManager->draw(delta);
           
                if (_statObserver) {
                    _statObserver->draw();
                }
            }
        }

        auto &commandBuffer = renderer->getRenderCommandBuffer();
        commandBuffer.begin();

        _graphicsDataUpdater->updateData();
        //_graphicsDataUpdater->updateData<GraphicsDataUpdateSystem<Mesh*>>();

        ///// shadow pass
        renderShadowMap(_shadowMap.get(), shadowRenderList, commandBuffer, currentFrame,
                        [this](engine::RenderedEntity* entity){
                                // need set correct program
                                auto mesh_skin_shadow_program = CascadeShadowMap::getShadowProgram<MeshSkinnedShadow>();
                                return entity->setProgram(mesh_skin_shadow_program, _shadowMap->getRenderPass());
                            }, [](engine::RenderedEntity* entity, vulkan::VulkanGpuProgram* program){
                                entity->setProgram(program);
                            }
        );
        ///// shadow pass

        std::array<VkClearValue, 2> clearValues = {
                VkClearValue{0.5f, 0.75f, 0.85f, 1.0f}, // for 1st attachment (color)
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

        // render nodes
        rootRenderList.render(commandBuffer, currentFrame,
                              {&mainCamera.getTransform(),
                               nullptr,
                               nullptr});

        uiRenderList.render(commandBuffer, currentFrame,
                            {nullptr,
                             nullptr,
                             nullptr});

        // draw bounding boxes
        constexpr bool kDrawBoundingVolumes = false;
        if constexpr (kDrawBoundingVolumes) {
            renderNodesBounds(_rootNode.get(), mainCamera.getTransform(), commandBuffer, currentFrame, 0u);
        }

        //Engine::getInstance().getModule<Graphics>().getRenderHelper()->drawSphere({0.0f, 0.0f, 0.5f}, 5.0f, mainCamera.getTransform(), makeMatrix(1.0f), commandBuffer, currentFrame, true);
        autoBatcher->draw(commandBuffer, currentFrame);

        commandBuffer.cmdEndRenderPass();
        commandBuffer.end();
    }
}