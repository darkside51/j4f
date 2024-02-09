#include "unit.h"

#include "../../graphics/scene.h"

#include <Engine/Graphics/Animation/AnimationManager.h>
#include <Engine/Graphics/Mesh/AnimationTree.h>

#include <Engine/Core/AssetManager.h>
#include <Engine/Graphics/GpuProgramsManager.h>
#include <Engine/Graphics/Mesh/Mesh.h>
#include <Engine/Graphics/Mesh/MeshLoader.h>

#include "../../service_locator.h"

#include <memory>

namespace game {

    Unit::Unit() : Entity(this), _animations(nullptr) {
        using namespace engine;

        auto && assetManager = Engine::getInstance().getModule<AssetManager>();
        auto && gpuProgramManager = Engine::getInstance().getModule<Graphics>().getGpuProgramsManager();

        const std::vector<engine::ProgramStageInfo> psi = {
                {ProgramStage::VERTEX, "resources/shaders/mesh_skin.vsh.spv"},
                {ProgramStage::FRAGMENT, "resources/shaders/mesh_skin.psh.spv"}
        };
        auto &&program = gpuProgramManager->getProgram(psi);

        MeshLoadingParams mesh_params;
        {
            using namespace gltf;
            mesh_params.file = "resources/assets/models/nuke_man/model.gltf";
            mesh_params.semanticMask = makeSemanticsMask(AttributesSemantic::POSITION, AttributesSemantic::NORMAL,
                                                         AttributesSemantic::JOINTS, AttributesSemantic::WEIGHT,
                                                         AttributesSemantic::TEXCOORD_0);
            mesh_params.latency = 3;
            mesh_params.flags->async = 1;
            mesh_params.callbackThreadId = 0;

            //meshesGraphicsBuffer = new MeshGraphicsDataBuffer(10 * 1024 * 1024, 10 * 1024 * 1024); // or create with default constructor for unique buffer for mesh
            mesh_params.graphicsBuffer = new MeshGraphicsDataBuffer();
        }

        auto scene = ServiceLocator::instance().getService<Scene>();
        auto mesh = std::make_unique<NodeRenderer<Mesh*>>();
        {
            assetManager.loadAsset<Mesh *>(mesh_params,
                                   [this, mesh = engine::make_ref(mesh), program, &assetManager](std::unique_ptr<Mesh> && asset, const AssetLoadingResult result) mutable {
                                       asset->setProgram(program);

                                       // animations
                                       _animations = std::make_unique<MeshAnimationTree>(0.0f, asset->getNodesCount(),
                                                                        asset->getSkeleton()->getLatency());

                                       auto && animationManager = Engine::getInstance().getModule<Graphics>().getAnimationManager();
                                       animationManager->registerAnimation(_animations.get());
                                       animationManager->addTarget(_animations.get(), asset->getSkeleton().get());

                                       MeshLoadingParams anim1;
                                       //anim1.file = "resources/assets/models/nuke_man/idle_angry.gltf";
                                       anim1.file = "resources/assets/models/nuke_man/running.gltf";
                                       anim1.flags->async = 1;
                                       anim1.callbackThreadId = 1;

                                       assetManager.loadAsset<Mesh*>(anim1,
                                                             [mainAsset = asset.get(), this](std::unique_ptr<Mesh>&& asset, const AssetLoadingResult result) {
                                                                 _animations->getAnimator()->addChild(new MeshAnimationTree::AnimatorType(
                                                                         &asset->getMeshData()->animations[0], 1.0f,
                                                                         mainAsset->getSkeleton()->getLatency()));
                                                             }
                                       );
                                       
                                       //////////////////////
                                       mat4f wtr = makeMatrix(50.0f);
                                       rotateMatrix_xyz(wtr, vec3f(0.0, 0.0, 0.0f));
                                       translateMatrixTo(wtr, vec3f(0.0f, -0.0f, 0.0f));

                                       auto && node = mesh->getNode();
                                       node->setLocalMatrix(wtr);
                                       //node->setBoundingVolume(BoundingVolume::make<SphereVolume>(vec3f(0.0f, 0.0f, 0.4f), 0.5f));
                                       mesh->setGraphics(asset.release());
                                   });
        }

        auto &&node = scene->placeToWorld(mesh.release()); // scene->placeToNode(mesh.release(), _mapNode); ??
    }

    Unit::~Unit() = default;

    void update(const float delta) {}

}