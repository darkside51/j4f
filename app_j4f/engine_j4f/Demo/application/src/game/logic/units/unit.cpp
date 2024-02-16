#include "unit.h"

#include "../../graphics/scene.h"

#include <Engine/Graphics/Animation/AnimationManager.h>
#include <Engine/Graphics/Mesh/AnimationTree.h>

#include <Engine/Core/AssetManager.h>
#include <Engine/Graphics/GpuProgramsManager.h>
#include <Engine/Graphics/Mesh/Mesh.h>
#include <Engine/Graphics/Mesh/MeshLoader.h>

#include <Engine/Graphics/Texture/TexturePtrLoader.h>

#include "../../service_locator.h"

#include <memory>

namespace game {

    engine::TexturePtr texture;
    auto meshGraphicsBuffer = new engine::MeshGraphicsDataBuffer();

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
            mesh_params.callbackThreadId = 1;

            //meshesGraphicsBuffer = new MeshGraphicsDataBuffer(10 * 1024 * 1024, 10 * 1024 * 1024); // or create with default constructor for unique buffer for mesh
            mesh_params.graphicsBuffer = meshGraphicsBuffer;
        }

        auto mesh = std::make_unique<NodeRenderer<Mesh*>>();
        {
            TexturePtrLoadingParams textureParams;
            textureParams.files = { "resources/assets/models/nuke_man/texture.png" };
            textureParams.flags->async = 1;
            textureParams.flags->use_cache = 1;
            textureParams.cacheName = "vulkan_logo";
            texture = assetManager.loadAsset<TexturePtr>(textureParams);

            assetManager.loadAsset<Mesh *>(mesh_params,
                                   [this, mesh = engine::make_ref(mesh), program, &assetManager](std::unique_ptr<Mesh> && asset, const AssetLoadingResult result) mutable {
                                       //
                                       asset->setProgram(program);
                                       asset->setParamByName("u_texture", texture.get(), false);

                                       // animations
                                       _animations = std::make_unique<MeshAnimationTree>(0.0f, asset->getNodesCount(),
                                                                        asset->getSkeleton()->getLatency());
                                       _animations->addObserver(this);

                                       auto && animationManager = Engine::getInstance().getModule<Graphics>().getAnimationManager();
                                       animationManager->registerAnimation(_animations.get());
                                       animationManager->addTarget(_animations.get(), asset->getSkeleton().get());

                                       {
                                           MeshLoadingParams anim;
                                           anim.file = "resources/assets/models/nuke_man/idle.gltf";
                                           anim.flags->async = 0;
                                           anim.callbackThreadId = 1;

                                           assetManager.loadAsset<Mesh *>(anim,
                                                                          [mainAsset = asset.get(), this](
                                                                                  std::unique_ptr<Mesh> &&asset,
                                                                                  const AssetLoadingResult result) {
                                                                              _animations->getAnimator()->addChild(
                                                                                      new MeshAnimationTree::AnimatorType(
                                                                                              &asset->getMeshData()->animations[0],
                                                                                              1.0f,
                                                                                              mainAsset->getSkeleton()->getLatency()));
                                                                          }
                                           );
                                       }

                                       {
                                           MeshLoadingParams anim;
                                           anim.file = "resources/assets/models/nuke_man/running.gltf";
                                           anim.flags->async = 0;
                                           anim.callbackThreadId = 1;

                                           assetManager.loadAsset<Mesh *>(anim,
                                                                          [mainAsset = asset.get(), this](
                                                                                  std::unique_ptr<Mesh> &&asset,
                                                                                  const AssetLoadingResult result) {
                                                                              _animations->getAnimator()->addChild(
                                                                                      new MeshAnimationTree::AnimatorType(
                                                                                              &asset->getMeshData()->animations[0],
                                                                                              0.0f,
                                                                                              mainAsset->getSkeleton()->getLatency()));
                                                                          }
                                           );
                                       }

                                       //////////////////////
                                       auto && node = mesh->getNode();
                                       node->setBoundingVolume(BoundingVolume::make<SphereVolume>(vec3f(0.0f, 0.0f, 0.4f), 0.42f));
                                       mesh->setGraphics(asset.release());
                                   });
        }

        auto scene = ServiceLocator::instance().getService<Scene>();
        auto &&node = scene->placeToWorld(mesh.release()); // scene->placeToNode(mesh.release(), _mapNode); ??
        scene->addShadowCastNode(node);
        _mapObject.assignNode(node);
        _mapObject.setScale(vec3f(50.0f));
    }

    Unit::~Unit() {
        ServiceLocator::instance().getService<Scene>()->removeShadowCastNode(_mapObject.getNode());
        texture = nullptr;
        delete meshGraphicsBuffer;
    }
    Unit::Unit(Unit &&) noexcept = default;

    void Unit::onEvent(engine::AnimationEvent event, const engine::MeshAnimator* animator) {
        using namespace engine;
        switch (event) {
            case AnimationEvent::NewLoop:
                break;
            case AnimationEvent::EndLoop:
                break;
            case AnimationEvent::Finish:
                break;
            default: break;
        }
    }

    void Unit::updateAnimationState(const float delta) {
        if (!_animations) return;
        constexpr float kAnimChangeSpeed = 4.0f;
        const float animChangeSpeed = kAnimChangeSpeed * delta;
        auto &animations = _animations->getAnimator()->children();
        if (animations.size() <= _currentAnimId) return;

        auto &currentAnim = animations[_currentAnimId]->value();
        if (currentAnim.getWeight() < 1.0f) {
            uint8_t i = 0u;
            for (auto &animation: animations) {
                if (i != _currentAnimId) {
                    auto &anim = animation->value();
                    anim.setWeight(std::max(anim.getWeight() - animChangeSpeed, 0.0f));
                }
                ++i;
            }
            currentAnim.setWeight(std::min(currentAnim.getWeight() + animChangeSpeed, 1.0f));
        }
    }

    void Unit::setState(const UnitState state) noexcept {
        switch (state) {
            case UnitState::Undefined :
                break;
            case UnitState::Idle :
                _currentAnimId = 0u;
                break;
            case UnitState::Walking :
                _currentAnimId = 1u;
                break;
            case UnitState::Running :
                _currentAnimId = 1u;
                break;
        }
        _state = state;
    }

    void Unit::update(const float delta) {
        using namespace engine;
        const auto & p = _mapObject.getPosition();
        if (p != _moveTarget) {
            const auto vec = _moveTarget - p;
            constexpr float kAngleSpeed = 16.0f;
            if (glm::dot(vec, vec) > kAngleSpeed) {
                const float angleSpeed = kAngleSpeed * delta;
                const auto direction = as_normalized(vec);
                constexpr float kEps = 0.1f;
                const float moveSpeed = kAngleSpeed * 5.5f * delta;
                if (compare(_direction, direction, kEps)) {
                    _direction = as_normalized(_direction + (direction - _direction) * angleSpeed);
                } else {
                    _direction = direction;
                }
                const float rz = atan2(_direction.x, -_direction.y);
                _mapObject.setRotation(vec3f(0.0f, 0.0f, rz));
                _mapObject.setPosition(p + _direction * moveSpeed);
                setState(UnitState::Running);
            } else {
                _moveTarget = p; // stop move
                setState(UnitState::Idle);
            }
        } else {
            setState(UnitState::Idle);
        }

        updateAnimationState(delta);
        _mapObject.updateTransform();
    }

}