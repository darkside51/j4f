#include "map.h"
#include "../graphics/scene.h"

#include <Engine/Graphics/Plane/Plane.h>
#include <Engine/Graphics/GpuProgramsManager.h>

#include <cstdint>
#include <vector>

#include "../service_locator.h"

namespace game {

    Map::Map() {
        makeMapNode();
    }

    Map::~Map() {}

    void Map::makeMapNode() {
        using namespace engine;

        auto scene = ServiceLocator::instance().getService<Scene>();
        _mapNode = scene->placeToWorld();

        const float size = 10000.0f;
        const float uvSize = 100.0f;

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
                    {ProgramStage::VERTEX, "resources/shaders/texture.vsh.spv"},
                    {ProgramStage::FRAGMENT, "resources/shaders/texture.psh.spv"}
            };
            auto &&program = gpuProgramManager->getProgram(psi);
            plane->setProgram(program);
        }

        auto &&planeNode = scene->placeToNode(plane.release(), _mapNode);

        mat4f transform(1.0f);
        translateMatrixTo(transform, vec3f(-size * 0.5f, -size * 0.5f, 0.0f));
        planeNode->value().setLocalMatrix(transform);
    }

    void Map::update(const float delta) {
        for (auto & obj : _objects) {
            obj->updateNodeTransform();
        }
    }

}