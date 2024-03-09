#include "graphics_factory.h"

#include <Engine/Graphics/Mesh/MeshLoader.h>

namespace game {

    GraphicsFactory::GraphicsFactory() {
        _meshGraphicsBuffers.emplace_back(
                std::make_unique<engine::MeshGraphicsDataBuffer>(10 * 1024 * 1024, 10 * 1024 * 1024));
    }

    GraphicsFactory::~GraphicsFactory() = default;

    void GraphicsFactory::loadObjects(std::string_view path, std::function<void()>&& onComplete) {
        using namespace engine;
        JsonLoadingParams jsParams(path);
        jsParams.flags->async = 1u;

        const Json js =
                engine::Engine::getInstance().getModule<engine::AssetManager>().loadAsset<Json>(
                        jsParams,
                        [this, onComplete = std::move(onComplete)]
                                (const Json &asset, const AssetLoadingResult result) mutable {
                            if (result == AssetLoadingResult::LOADING_SUCCESS) {
                                loadObjects(asset, std::move(onComplete));
                            }
                        });
    }

    void GraphicsFactory::loadObjects(const engine::Json & json, std::function<void()>&& onComplete) {
        using namespace engine;
        using namespace gltf;
        // load gpu programs
        if (auto array = json.find("gpu_programs"); array != json.end()) {
            _gpuPrograms.reserve(_gpuPrograms.size() + array->size());
            for (auto &js: *array) {
                GPUProgramDescription description;
                description.name = js.value("name", "");
                for (auto &sh: js["programs"]) {
                    auto const type = sh.value("type", 0);
                    auto const path = sh.value("path", "");
                    description.modules[type] = path;
                }
                _gpuPrograms.emplace_back(std::move(description));
            }
        }

        // load animations
        if (auto array = json.find("animations"); array != json.end()) {
            _animations.reserve(_animations.size() + array->size());
            for (auto &js: *array) {
                auto const name = js.value("name", "");
                auto const path = js.value("path", "");
                auto const speed = js.value("speed", 1.0f);
                auto const infinity = js.value("infinity", true);

                _animations.emplace_back(AnimationDescription{name, path, speed, infinity});
            }
        }

        auto loadAnimations = [this](auto && animations,
                                                std::vector<std::tuple<size_t, float, uint8_t>> & anims){
            auto getAnimId = [this](std::string_view name) -> size_t{
                auto it = std::find_if(_animations.begin(), _animations.end(),
                                       [name](auto const & anim){
                                           return anim.name == name;
                                       });
                if (it != _animations.end()) {
                    return std::distance(_animations.begin(), it);
                }

                return 0u;
            };

            for (auto &animJs: *animations) {
                auto const name = animJs.value("name", "");
                auto const weight = animJs.value("weight", 0.0f);
                auto const id = static_cast<uint8_t>(animJs.value("id", 0));

                anims.emplace_back(getAnimId(name), weight, id);
            }
        };

        // animation sets
        if (auto array = json.find("animation_sets"); array != json.end()) {
            for (auto const & js: *array) {
                auto const & name = js.value("name", "");
                loadAnimations(js.find("animations"), _animationSets[name]);
            }
        }

        // meshes
        if (auto array = json.find("meshes"); array != json.end()) {
            auto const defaultSemanticMask = makeSemanticsMask(AttributesSemantic::POSITION, AttributesSemantic::NORMAL,
                                                               AttributesSemantic::JOINTS, AttributesSemantic::WEIGHT,
                                                               AttributesSemantic::TEXCOORD_0);
            _meshes.reserve(_meshes.size() + array->size());
            for (auto &js: *array) {
                auto const id = static_cast<uint16_t>(js.value("id", 0));
                auto const path = js.value("path", "");
                auto const latency = static_cast<uint8_t>(js.value("latency", 1));
                auto const semanticMask = js.value("semanticMask", defaultSemanticMask);
                auto const graphicsBufferId = js.value("graphicsBuffer", 0u);

                _meshes.emplace_back(id, std::string(path), latency, semanticMask, graphicsBufferId);
            }
        }

        // graphics
        if (auto array = json.find("graphics"); array != json.end()) {
            for (auto &js: *array) {
                auto const name = js.value("name", "");
                auto const type = static_cast<uint8_t>(js.value("type", 0));
                auto const gpuProgramName = js.value("gpu_program", "");

                GraphicsDescription graphics;
                graphics.type = static_cast<GraphicsType>(type);
                graphics.descriptor = static_cast<uint8_t>(js.value("descriptor", 0));

                auto it = std::find_if(_gpuPrograms.begin(), _gpuPrograms.end(),
                             [&gpuProgramName](auto const & p){
                                        return p.name == gpuProgramName;
                });

                if (it != _gpuPrograms.end()) {
                    graphics.gpuProgramId = std::distance(_gpuPrograms.begin(), it);
                }

                // animation set
                if (auto animationsSet = js.find("animation_set"); animationsSet != js.end()) {
                    auto const & key = animationsSet->get<std::string>();
                    if (auto it = _animationSets.find(key); it != _animationSets.end()) {
                        graphics.animations = &it->second;
                    }
                }

                if (auto drawParams = js.find("drawParams"); drawParams != js.end()) {
                    graphics.drawParams.cullMode =
                            static_cast<vulkan::CullMode>(drawParams->value("cullMode", 0));
                    graphics.drawParams.blendMode =
                            vulkan::VulkanBlendMode(drawParams->value("blendMode", 0));
                    graphics.drawParams.depthTest = drawParams->value("depthTest", false);
                    graphics.drawParams.depthWrite = drawParams->value("depthWrite", false);
                    graphics.drawParams.stencilTest = drawParams->value("stencilTest", false);
                    graphics.drawParams.stencilWrite = drawParams->value("stencilWrite", false);
                    graphics.drawParams.stencilRef = drawParams->value("stencilRef", 0);
                    graphics.drawParams.stencilFunction = drawParams->value("stencilFunction", 0);
                }

                if (auto textures = js.find("textures"); textures != js.end()) {
                    graphics.textures.reserve(textures->size());
                    for (auto & texture: *textures) {
                        auto const name = texture.get<std::string_view>();
                        auto it = std::find(_textures.begin(), _textures.end(), name);
                        if (it != _textures.end()) {
                            graphics.textures.push_back(std::distance(_textures.begin(), it));
                        } else {
                            graphics.textures.push_back(_textures.size());
                            _textures.emplace_back(name);
                        }
                    }
                }

                _descriptions.emplace(name, std::move(graphics));
            }
        }

        // objects
        if (auto array = json.find("objects"); array != json.end()) {
            std::function<ObjectDescription(const Json &)> loadObject;
            loadObject = [this, &loadObject](const Json & json)-> ObjectDescription{

                auto const parseVec3 = [](const Json & json,
                        std::string_view name, vec3f defaultValue = {0.0f, 0.0f, 0.0f})-> vec3f {
                    if (auto array = json.find(name); array != json.end()) {
                        return {(*array)[0].get<float>(), (*array)[1].get<float>(), (*array)[2].get<float>()};
                    } else {
                        return defaultValue;
                    }
                };

                auto const graphics = json.value("graphics", "");
                auto const order = static_cast<uint16_t>(json.value("order", 0));

                ObjectDescription description = {graphics, order};

                description.rotationsOrder =
                        static_cast<RotationsOrder>(json.value("rotationOrder", 0));
                description.scale = parseVec3(json, "scale", {1.0f, 1.0f, 1.0f});
                description.rotation = parseVec3(json, "rotation");
                description.position = parseVec3(json, "position");

                if (auto array = json.find("children"); array != json.end()) {
                    description.children.reserve(array->size());
                    for (auto &js: *array) {
                        description.children.push_back(loadObject(js));
                    }
                }
                return description;
            };

            for (auto &js: *array) {
                auto const name = js.value("name", "");
                _objects.emplace(name, loadObject(js));
            }
        }

        if (onComplete) {
            onComplete();
        }
    }
}