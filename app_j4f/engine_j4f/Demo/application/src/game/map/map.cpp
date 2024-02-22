#include "map.h"
#include "../graphics/scene.h"

#include <Engine/Graphics/Plane/Plane.h>
#include <Engine/Graphics/GpuProgramsManager.h>
#include <Engine/Graphics/Features/Shadows/CascadeShadowMap.h>
#include <Engine/Graphics/Texture/TexturePtrLoader.h>

#include <cstdint>
#include <vector>

#include "../service_locator.h"

#include "../logic/units/unit.h"

namespace game {

    engine::TexturePtr textureGround;

    Map::Map() {
        makeMapNode();
    }

    Map::~Map() {
        textureGround = nullptr;
    }

    void Map::makeMapNode() {
        using namespace engine;

        auto scene = ServiceLocator::instance().getService<Scene>();
        _mapNode = scene->placeToWorld();

        const float size = 50000.0f;
        const float uvSize = 200.0f;

        auto planeGraphics = std::make_unique<Plane>(
                std::make_shared<TextureFrame>(
                        std::vector<float>{
                                0.0f, 0.0f,
                                size, 0.0f,
                                0.0f, size,
                                size, size
                        },
                        std::vector<float>{
                                0.0f, uvSize,
                                uvSize, uvSize,
                                0.0f, 0.0f,
                                uvSize, 0.0f
                        },
                        std::vector<uint32_t>{
                                0u, 1u, 2u, 2u, 1u, 3u
                        }
                ), nullptr);

        planeGraphics->changeRenderState([](vulkan::VulkanRenderState &renderState) {
            renderState.blendMode = vulkan::CommonBlendModes::blend_none;
            renderState.depthState.depthWriteEnabled = true;
            renderState.depthState.depthTestEnabled = true;
        });

        auto plane = std::make_unique<NodeRenderer<Plane *>>();
        plane->setGraphics(planeGraphics.release());

        {
            auto &&gpuProgramManager = Engine::getInstance().getModule<Graphics>().getGpuProgramsManager();
            const std::vector<engine::ProgramStageInfo> psi = {
                    {ProgramStage::VERTEX, "resources/shaders/shadows_plane.vsh.spv"},
                    {ProgramStage::FRAGMENT, "resources/shaders/shadows_plane.psh.spv"}
            };
            auto &&program = gpuProgramManager->getProgram(psi);
            plane->setProgram(program);

            plane->graphics()->setParamByName("u_shadow_map", scene->getShadowMap()->getTexture(), false);
            scene->getShadowMap()->registerProgramAsReciever(program);
        }

        auto plainPtr = plane.release();
        auto &&planeNode = scene->placeToNode(plainPtr, _mapNode);

        mat4f transform(1.0f);
        translateMatrixTo(transform, vec3f(-size * 0.5f, -size * 0.5f, 0.0f));
        planeNode->value().setLocalMatrix(transform);

        /////////
        auto && assetManager = Engine::getInstance().getModule<AssetManager>();
        TexturePtrLoadingParams textureParams;
        textureParams.files = { "resources/assets/textures/ground/sand1.jpg" };
        textureParams.flags->async = 1;
        textureParams.flags->use_cache = 1;
        textureParams.callbackThreadId = 0u;

        textureGround = assetManager.loadAsset<TexturePtr>(textureParams, [](TexturePtr const & asset, const AssetLoadingResult result) {
            auto&& renderer = Engine::getInstance().getModule<Graphics>().getRenderer();
            const_cast<TexturePtr&>(asset)->get()->setSampler(
                    renderer->getSampler(
                            VK_FILTER_LINEAR,
                            VK_FILTER_LINEAR,
                            VK_SAMPLER_MIPMAP_MODE_LINEAR,
                            VK_SAMPLER_ADDRESS_MODE_REPEAT,
                            VK_SAMPLER_ADDRESS_MODE_REPEAT,
                            VK_SAMPLER_ADDRESS_MODE_REPEAT,
                            VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK
                    ));
        });

        plainPtr->graphics()->setParamByName("u_texture", textureGround.get(), false);
    }

    void Map::update(const float delta) {}

}