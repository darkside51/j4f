#include "unit.h"

#include "../../graphics/scene.h"

#include <Engine/Graphics/Animation/AnimationManager.h>
#include <Engine/Graphics/Mesh/AnimationTree.h>

#include <Engine/Core/AssetManager.h>
#include <Engine/Graphics/GpuProgramsManager.h>
#include <Engine/Graphics/Mesh/Mesh.h>
#include <Engine/Graphics/Mesh/MeshLoader.h>
#include <Engine/Core/Threads/WorkersCommutator.h>
#include <Engine/Utils/StringHelper.h>

#include <Engine/Graphics/Texture/TexturePtrLoader.h>
#include <Engine/Graphics/Color/Color.h>
#include <Engine/Core/Math/random.h>

#include "../../service_locator.h"
#include "../../graphics/graphics_factory.h"

#include <memory>

namespace game {

    Unit::Unit(std::string_view name) : Entity(this), _animations(nullptr) {
        using namespace engine;
        auto &&serviceLocator = ServiceLocator::instance();

        auto &&assetManager = Engine::getInstance().getModule<AssetManager>();
        auto &&gpuProgramManager = Engine::getInstance().getModule<Graphics>().getGpuProgramsManager();

        auto graphicsFactory = serviceLocator.getService<GraphicsFactory>();

        auto it = graphicsFactory->_objects.find(name.data());
        if (it != graphicsFactory->_objects.end()) {
            auto const &descriptions = graphicsFactory->_descriptions;
            auto const &gpuPrograms = graphicsFactory->_gpuPrograms;
            auto const &animations = graphicsFactory->_animations;
            auto const &meshes = graphicsFactory->_meshes;
            auto const &textures = graphicsFactory->_textures;
            auto const &meshGraphicsBuffers = graphicsFactory->_meshGraphicsBuffers;

            auto const &object = it->second;
            auto const &description = descriptions.at(object.description);

            auto const gpuProgramDescription = gpuPrograms[description.gpuProgramId];

            std::vector<engine::ProgramStageInfo> psi;
            for (size_t i = 0u; i < gpuProgramDescription.modules.size(); ++i) {
                if (!gpuProgramDescription.modules[i].empty()) {
                    ProgramStage stage;
                    switch (static_cast<GPUProgramDescription::Module>(i)) {
                        case GPUProgramDescription::Module::Vertex:
                            stage = ProgramStage::VERTEX;
                            break;
                        case GPUProgramDescription::Module::Fragment:
                            stage = ProgramStage::FRAGMENT;
                            break;
                        case GPUProgramDescription::Module::Geometry:
                            stage = ProgramStage::GEOMETRY;
                            break;
                        case GPUProgramDescription::Module::Compute:
                            stage = ProgramStage::COMPUTE;
                            break;
                    }
                    psi.emplace_back(stage, "resources/shaders/" + gpuProgramDescription.modules[i]);
                }
            }

            auto && program = gpuProgramManager->getProgram(psi);

            NodePtr node;
            auto scene = serviceLocator.getService<Scene>();

            /////// texture loading
            std::vector<TexturePtr> objectTextures;
            objectTextures.reserve(description.textures.size());
            for (const auto texId : description.textures) {
                TexturePtrLoadingParams textureParams;
                textureParams.files = {"resources/assets/" + textures[texId]};
                textureParams.flags->async = 1u;
                textureParams.flags->use_cache = 1u;
                textureParams.callbackThreadId = static_cast<uint8_t>(Engine::Workers::RENDER_THREAD);
                objectTextures.push_back(assetManager.loadAsset<TexturePtr>(textureParams));
            }
            /////// texture loading

            switch (description.type) {
                case GraphicsType::Mesh: {
                    auto const &meshDescriptor = meshes[description.descriptor];

                    const uint8_t latency = meshDescriptor.latency;
                    MeshLoadingParams mesh_params;
                    mesh_params.file = "resources/assets/" + meshDescriptor.path;
                    mesh_params.semanticMask = meshDescriptor.semanticMask;
                    mesh_params.latency = latency;
                    mesh_params.flags->async = 1u;
                    mesh_params.callbackThreadId = static_cast<uint8_t>(Engine::Workers::UPDATE_THREAD);

                    // or create with default constructor for unique buffer for mesh
                    mesh_params.graphicsBuffer = meshGraphicsBuffers[meshDescriptor.graphicsBufferId];

                    auto mesh = std::make_unique<NodeRenderer<Mesh*>>();
                    auto meshPtr = mesh.release();
                    node = scene->placeToWorld(meshPtr); // scene->placeToNode(mesh.release(), _mapNode); ??

                    auto loadAnim = [&assetManager, this] (
                            ref_ptr<Mesh> mainAsset, const std::string file, uint8_t id, float weight, bool infinity,
                            float speed){
                        MeshLoadingParams anim;
                        anim.file = "resources/assets/" + file;
                        anim.flags->async = 1u;
                        anim.callbackThreadId = static_cast<uint8_t>(Engine::Workers::UPDATE_THREAD);

                        assetManager.loadAsset<Mesh*>(
                                anim,[mainAsset, this, id, weight, speed, infinity] (
                                        std::unique_ptr<Mesh> && asset,
                                        const AssetLoadingResult result) mutable {
                                    _animations->getAnimator()->assignChild(
                                            id,
                                            new MeshAnimationTree::AnimatorType(
                                                    &asset->getMeshData()->animations[0],
                                                    weight, mainAsset->getSkeleton()->getLatency(),
                                                    speed, infinity));
                                }
                        );
                    };

                    assetManager.loadAsset<Mesh *>(
                            mesh_params,
                            [this, &drawParams = description.drawParams,
                             &animations = description.animations,
                             &allAnimations = animations,
                             order = object.order,
                             loadAnim = std::move(loadAnim),
                             objectTextures,
                             mesh = engine::make_ref(meshPtr), program,
                             &assetManager, &gpuProgramManager
                             ] (
                                     std::unique_ptr<Mesh> &&asset,
                                     const AssetLoadingResult result) mutable {
                                        asset->setProgram(program);

                                        // set textures
                                        uint8_t slot = 0u;
                                        for (auto & texture : objectTextures) {
                                            asset->setParamByName(
                                                    fmtString("u_texture_{}", slot++), texture.get(),
                                                    false);
                                        }

                                        asset->changeRenderState(
                                                [&drawParams](vulkan::VulkanRenderState &renderState) {
                                                    renderState.rasterizationState.cullMode = drawParams.cullMode;
                                                    renderState.blendMode = drawParams.blendMode;
                                                    renderState.depthState.depthTestEnabled = drawParams.depthTest;
                                                    renderState.depthState.depthWriteEnabled = drawParams.depthWrite;
                                                    const uint8_t stencilWriteMsk = drawParams.stencilWrite ? 0xffu : 0x0u;
                                                    renderState.stencilState =
                                                            vulkan::VulkanStencilState{
                                                                drawParams.stencilTest,VK_STENCIL_OP_KEEP,
                                                                VK_STENCIL_OP_REPLACE,VK_STENCIL_OP_KEEP,
                                                                static_cast<VkCompareOp>(drawParams.stencilFunction),
                                                                0xffu, stencilWriteMsk,
                                                                drawParams.stencilRef};
                                                });

                                        // animations
                                        if (!animations.empty()) {
                                            _animations = std::make_unique<MeshAnimationTree>(0.0f,
                                                                                              asset->getNodesCount(),
                                                                                              asset->getSkeleton()->getLatency());
                                            _animations->addObserver(this);
                                            _animations->getAnimator()->resize(animations.size());

                                            auto && animationManager =
                                                    Engine::getInstance().getModule<Graphics>().getAnimationManager();
                                            animationManager->registerAnimation(_animations.get());
                                            animationManager->addTarget(_animations.get(),
                                                                        asset->getSkeleton().get());

                                            for (const auto &anim: animations) {
                                                const auto animId = std::get<size_t>(anim);
                                                const auto weight = std::get<float>(anim);
                                                const auto id = std::get<uint8_t>(anim);

                                                const auto &animation = allAnimations[animId];
                                                loadAnim(asset, animation.path, id, weight,
                                                         animation.infinity, animation.speed);
                                            }
                                        }

                                        auto &&node = _mapObject.getNode()->value();
                                        node.setBoundingVolume(BoundingVolume::make<SphereVolume>(
                                                vec3f(0.0f, 0.0f, 0.4f),0.42f));
                                        auto meshGraphics = asset.release();
                                        mesh->setGraphics(meshGraphics);
                                        mesh->getRenderDescriptor().order = order;

//                                       {
//                                           // test add refEntity
//                                           auto meshRef = std::make_unique<NodeRenderer<ReferenceEntity<Mesh> *>>();
//                                           auto g = new ReferenceEntity<Mesh>(meshGraphics);
//                                           g->setProgram(program);
//
//                                           g->changeRenderState([](vulkan::VulkanRenderState& renderState) {
//                                               renderState.rasterizationState.cullMode = vulkan::CullMode::CULL_MODE_NONE;
//                                               renderState.stencilState =
//                                                       vulkan::VulkanStencilState{true, VK_STENCIL_OP_KEEP,
//                                                                                  VK_STENCIL_OP_REPLACE,
//                                                                                  VK_STENCIL_OP_KEEP,
//                                                                                  VK_COMPARE_OP_ALWAYS,
//                                                                                  0xffu, 0xffu, 1u};
//                                           });
//
//                                           g->setParamByName("u_texture", texture.get(), false);
//                                           meshRef->setGraphics(g);
//                                           meshRef->getRenderDescriptor().order = 1u;
//                                           auto &&serviceLocator = ServiceLocator::instance();
//                                           auto scene = serviceLocator.getService<Scene>();
//                                           auto &&node2 = scene->placeToNode(meshRef.release(), _mapObject.getNode());
//                                           scene->addShadowCastNode(node2);
//                                           auto matrix = engine::makeMatrix(1.0f);
//                                           engine::translateMatrixTo(matrix, {1.0f, 0.0f, 0.0f});
//                                           node2->value().setLocalMatrix(matrix);
//                                       }

                                {
                                    // test add refEntity
                                    const std::vector<engine::ProgramStageInfo> psi = {
                                            {ProgramStage::VERTEX,   "resources/shaders/mesh_skin_stroke.vsh.spv"},
                                            {ProgramStage::FRAGMENT, "resources/shaders/color_param.psh.spv"}
                                    };
                                    auto &&program_stroke = gpuProgramManager->getProgram(psi);

                                    auto meshRef = std::make_unique<NodeRenderer<ReferenceEntity<Mesh> *>>();
                                    auto g = new ReferenceEntity<Mesh>(meshGraphics);
                                    g->setProgram(program_stroke);

                                    Color color(random(0u, 0xffffffffu));
                                    g->setParamByName("color", color.vec4(), true);

                                    g->changeRenderState([&drawParams](vulkan::VulkanRenderState &renderState) {
                                        renderState.rasterizationState.cullMode = vulkan::CullMode::CULL_MODE_BACK;
                                        renderState.depthState.depthTestEnabled = false;
                                        renderState.depthState.depthWriteEnabled = true; // if usestroke on one object
                                        //renderState.depthState.depthWriteEnabled = false; //
                                        renderState.stencilState =
                                                vulkan::VulkanStencilState{true, VK_STENCIL_OP_KEEP,
                                                                           VK_STENCIL_OP_KEEP,
                                                                           VK_STENCIL_OP_KEEP,
                                                                           VK_COMPARE_OP_NOT_EQUAL,
                                                                           0xffu, 0x0u,
                                                                           drawParams.stencilRef};
                                    });
                                    meshRef->setGraphics(g);
                                    meshRef->getRenderDescriptor().order = 0u; // same order
                                    auto &&serviceLocator = ServiceLocator::instance();
                                    auto scene = serviceLocator.getService<Scene>();
                                    auto &&node2 = scene->placeToNode(meshRef.release(),
                                                                      _mapObject.getNode());
                                    auto matrix = engine::makeMatrix(1.0f);
                                    node2->value().setLocalMatrix(matrix);
                                }
                            });

                }
                    break;
                default:
                    break;
            }

            if (node) {
                scene->addShadowCastNode(node);
                _mapObject.assignNode(node);
                for (const auto & texture : objectTextures) {
                    _mapObject.addTexture(texture);
                }
                _mapObject.setRotationsOrder(object.rotationsOrder);
                _mapObject.setScale(object.scale);
                _mapObject.setRotation(object.rotation);
                _mapObject.setPosition(object.position);
            }
        }

    }

    Unit::~Unit() {
        ServiceLocator::instance().getService<Scene>()->removeShadowCastNode(_mapObject.getNode());
    }

    Unit::Unit(Unit &&) noexcept = default;

    void Unit::setPosition(const engine::vec3f & p) noexcept {
        _mapObject.setPosition(p);
        _moveTarget = p;
    }

    void Unit::setRotation(const engine::vec3f & r) noexcept {
        _mapObject.setRotation(r);
    }

    engine::vec3f Unit::getPosition() const noexcept {
        using namespace engine;
        if (Engine::getInstance().getModule<WorkerThreadsCommutator>().checkCurrentThreadIs(
                static_cast<uint8_t>(Engine::Workers::RENDER_THREAD))) {
            // for render thread
            const auto &m = _mapObject.getTransform();
            return {m[3][0], m[3][1], m[3][2]};
        } else {
            // for update thread
            return _mapObject.getPosition();
        }
    }

    void Unit::onEvent(engine::AnimationEvent event, const engine::MeshAnimator *animator) {
        using namespace engine;
        switch (event) {
            case AnimationEvent::NewLoop:
                break;
            case AnimationEvent::EndLoop:
                break;
            case AnimationEvent::Finish:
                setState(UnitState::Idle, _currentAnimId == 6u ? 1u : 0u);
                break;
            default:
                break;
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
            for (auto & animation: animations) {
                if (animation && i != _currentAnimId) {
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

    void Unit::update(const float delta, const engine::Camera& camera) {
        using namespace engine;
        const auto &p = _mapObject.getPosition();

        if (p != _moveTarget) {
            const auto vec = _moveTarget - p;
            constexpr float kAngleSpeed = 12.0f;
            if (glm::dot(vec, vec) > kAngleSpeed) {
                const float angleSpeed = kAngleSpeed * delta;
                const auto direction = as_normalized(vec);

                const uint8_t moveAimId = 2u;
                float weight = 1.0f;
                if (_animations) {
                    const auto &animations = _animations->getAnimator()->children();
                    if (auto && animation = animations[moveAimId]) {
                        auto && anim = animation->value();
                        weight = anim.getWeight();
                    }
                }

                const float moveSpeed = kAngleSpeed * 7.7f * delta * weight;
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

        { //// todo!
            const auto length = engine::vec_length(camera.getPosition() - _mapObject.getPosition());
            if (auto&& r = _mapObject.getNode()->value().getRenderer(); r && r->getRenderEntity()) {
                r->getRenderDescriptor().order = length;
            }

            if (!_mapObject.getNode()->children().empty()) {
                if (auto&& r = _mapObject.getNode()->children()[0]->value().getRenderer(); r && r->getRenderEntity()) {
                    r->getRenderDescriptor().order = length;
                }
            }
        }

    }

}