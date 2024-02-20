#include "unit.h"

#include "../../graphics/scene.h"

#include <Engine/Graphics/Animation/AnimationManager.h>
#include <Engine/Graphics/Mesh/AnimationTree.h>

#include <Engine/Core/AssetManager.h>
#include <Engine/Graphics/GpuProgramsManager.h>
#include <Engine/Graphics/Mesh/Mesh.h>
#include <Engine/Graphics/Mesh/MeshLoader.h>
#include <Engine/Core/Threads/WorkersCommutator.h>

#include <Engine/Graphics/Texture/TexturePtrLoader.h>

#include "../../service_locator.h"

#include <memory>

namespace game {

    engine::TexturePtr texture;

    Unit::Unit(engine::ref_ptr<engine::MeshGraphicsDataBuffer> graphicsBuffer) : Entity(this), _animations(nullptr) {
        using namespace engine;
        auto && serviceLocator = ServiceLocator::instance();

        auto && assetManager = Engine::getInstance().getModule<AssetManager>();
        auto && gpuProgramManager = Engine::getInstance().getModule<Graphics>().getGpuProgramsManager();

        const std::vector<engine::ProgramStageInfo> psi = {
                {ProgramStage::VERTEX, "resources/shaders/mesh_skin.vsh.spv"},
                {ProgramStage::FRAGMENT, "resources/shaders/mesh_skin.psh.spv"}
        };
        auto &&program = gpuProgramManager->getProgram(psi);

        const uint8_t latency = 3u;
        MeshLoadingParams mesh_params;
        {
            using namespace gltf;
            mesh_params.file = "resources/assets/models/nuke_man/model.gltf";
            mesh_params.semanticMask = makeSemanticsMask(AttributesSemantic::POSITION, AttributesSemantic::NORMAL,
                                                         AttributesSemantic::JOINTS, AttributesSemantic::WEIGHT,
                                                         AttributesSemantic::TEXCOORD_0);
            mesh_params.latency = latency;
            mesh_params.flags->async = 1;
            mesh_params.callbackThreadId = 1;

            // or create with default constructor for unique buffer for mesh
            mesh_params.graphicsBuffer = graphicsBuffer;
        }

        auto mesh = std::make_unique<NodeRenderer<Mesh*>>();
        {
            TexturePtrLoadingParams textureParams;
            textureParams.files = { "resources/assets/models/nuke_man/texture.png" };
            textureParams.flags->async = 1;
            textureParams.flags->use_cache = 1;
            texture = assetManager.loadAsset<TexturePtr>(textureParams);

            auto meshPtr = mesh.release();
            auto scene = serviceLocator.getService<Scene>();
            auto &&node = scene->placeToWorld(meshPtr); // scene->placeToNode(mesh.release(), _mapNode); ??
            scene->addShadowCastNode(node);
            _mapObject.assignNode(node);
            _mapObject.setScale(vec3f(50.0f));

            assetManager.loadAsset<Mesh *>(mesh_params,
                                   [this, mesh = engine::make_ref(meshPtr), program,
                                    &assetManager](
                                           std::unique_ptr<Mesh> && asset, const AssetLoadingResult result) mutable {
                                       asset->setProgram(program);
                                       asset->setParamByName("u_texture", texture.get(), false);

                                       // animations
                                       _animations = std::make_unique<MeshAnimationTree>(0.0f, asset->getNodesCount(),
                                                                        asset->getSkeleton()->getLatency());
                                       _animations->addObserver(this);
                                       _animations->getAnimator()->resize(7u);

                                       auto && animationManager = Engine::getInstance().getModule<Graphics>().getAnimationManager();
                                       animationManager->registerAnimation(_animations.get());
                                       animationManager->addTarget(_animations.get(), asset->getSkeleton().get());

                                       const auto loadAnim =
                                               [&assetManager, this, mainAsset = asset.get()](
                                                       const std::string file, uint8_t id, float weight,
                                                       bool infinity = true, float speed = 1.0f) {
                                           MeshLoadingParams anim;
                                           anim.file = "resources/assets/models/" + file;
                                           anim.flags->async = 1;
                                           anim.callbackThreadId = 1;

                                           assetManager.loadAsset<Mesh*>(anim,
                                               [mainAsset, this, id, weight, speed, infinity](
                                                   std::unique_ptr<Mesh>&& asset,
                                                   const AssetLoadingResult result) mutable {
                                                       _animations->getAnimator()->assignChild(id,
                                                       new MeshAnimationTree::AnimatorType(
                                                           &asset->getMeshData()->animations[0],
                                                           weight,
                                                           mainAsset->getSkeleton()->getLatency(), speed, infinity));
                                               }
                                           );
                                       };

                                       loadAnim("nuke_man/idle.gltf", 0u, 1.0f);
                                       loadAnim("nuke_man/idle_angry.gltf", 1u, 0.0f);
                                       loadAnim("nuke_man/running.gltf", 2u, 0.0f);
                                       loadAnim("nuke_man/yelling.gltf", 3u, 0.0f, false);
                                       loadAnim("nuke_man/hokey_pokey.gltf", 4u, 0.0f, false);
                                       loadAnim("nuke_man/dancing0.gltf", 5u, 0.0f, false);
                                       loadAnim("nuke_man/kick0.gltf", 6u, 0.0f, false);

                                       //////////////////////
                                       auto && node = _mapObject.getNode()->value();
                                       node.setBoundingVolume(BoundingVolume::make<SphereVolume>(vec3f(0.0f, 0.0f, 0.4f), 0.42f));
                                       mesh->setGraphics(asset.release());
                                   });
        }
    }

    Unit::~Unit() {
        ServiceLocator::instance().getService<Scene>()->removeShadowCastNode(_mapObject.getNode());
        texture = nullptr;
    }

    Unit::Unit(Unit &&) noexcept = default;

    engine::vec3f Unit::getPosition() const {
        using namespace engine;
        if (Engine::getInstance().getModule<WorkerThreadsCommutator>().checkCurrentThreadIs(static_cast<uint8_t>(Engine::Workers::RENDER_THREAD))) {
            // for render thread
            const auto& m = _mapObject.getTransform();
            return { m[3][0], m[3][1], m[3][2] };
        } else {         
            // for update thread
            return _mapObject.getPosition();
        }        
    }

    void Unit::onEvent(engine::AnimationEvent event, const engine::MeshAnimator* animator) {
        using namespace engine;
        switch (event) {
            case AnimationEvent::NewLoop:
                break;
            case AnimationEvent::EndLoop:
                break;
            case AnimationEvent::Finish:
                setState(UnitState::Idle, _currentAnimId == 6u ? 1u : 0u);
                break;
            default: break;
        }
    }

    void Unit::updateAnimationState(const float delta) {
        if (!_animations) return;

        // if unit is alive
        _animations->activate();

        constexpr float kAnimChangeSpeed = 4.0f;
        const float animChangeSpeed = kAnimChangeSpeed * delta;
        auto &animations = _animations->getAnimator()->children();
        if (animations.size() <= _currentAnimId || !animations[_currentAnimId]) return;

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

    void Unit::setState(const UnitState state, uint8_t specialAnimId) noexcept {
        switch (state) {
            case UnitState::Undefined :
                break;
            case UnitState::Idle :
                _currentAnimId = specialAnimId;
                break;
            case UnitState::Walking :
                _currentAnimId = 1u;
                break;
            case UnitState::Running :
                _currentAnimId = 2u;
                break;
            case UnitState::Special:
                _moveTarget = _mapObject.getPosition();
                _currentAnimId = specialAnimId;
                break;
            default:
                break;
        }

        _state = state;
    }

    void Unit::update(const float delta) {
        using namespace engine;
        const auto & p = _mapObject.getPosition();

        if (p != _moveTarget) {
            const auto vec = _moveTarget - p;
            constexpr float kAngleSpeed = 12.0f;
            if (glm::dot(vec, vec) > kAngleSpeed) {
                const float angleSpeed = kAngleSpeed * delta;
                const auto direction = as_normalized(vec);
                const float moveSpeed = kAngleSpeed * 7.0f * delta;
                const auto sub = (direction - _direction);
                if (vec_length(sub) > angleSpeed) {
                    _direction = as_normalized(_direction + as_normalized(sub) * angleSpeed);
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
        } else if (_state == UnitState::Running) {
            setState(UnitState::Idle);
        }

        updateAnimationState(delta);
        _mapObject.updateTransform();
    }

}