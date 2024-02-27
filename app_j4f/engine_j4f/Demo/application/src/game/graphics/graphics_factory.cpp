#include "graphics_factory.h"

namespace game {

//    template <typename T, typename F, typename... Args>
//    static void parseArray(std::vector<T>& arr, F&& f, const std::string& name, const engine::Json & js, Args&&... args) {
//        if (auto targetJs = js.find(name); targetJs != js.end()) {
//            const size_t count = targetJs->size();
//            arr.resize(count);
//            for (size_t i = 0; i < count; ++i) {
//                f(arr[i], (*targetJs)[i], std::forward<Args>(args)...);
//            }
//        }
//    }

    void GraphicsFactory::loadObjects(std::string_view path) {
        using namespace engine;
        JsonLoadingParams jsParams(path);
        jsParams.flags->async = 1u;

        const Json js =
                engine::Engine::getInstance().getModule<engine::AssetManager>().loadAsset<Json>(
                        jsParams,[this](const Json &asset, const AssetLoadingResult result) {
                            if (result == AssetLoadingResult::LOADING_SUCCESS) {
                                loadObjects(asset);
                            }
                        });
    }

    void GraphicsFactory::loadObjects(const engine::Json & json) {
        // load gpu programs
        if (auto array = json.find("gpu_programs"); array != json.end()) {
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
            for (auto &js: *array) {
                auto const name = js.value("name", "");
                auto const path = js.value("path", "");
                auto const speed = js.value("speed", 1.0f);
                auto const infinity = js.value("infinity", true);

                _animations.emplace_back(AnimationDescription{name, path, speed, infinity});
            }
        }

        // meshes
        if (auto array = json.find("meshes"); array != json.end()) {
            _meshes.reserve(array->size());
            for (auto &js: *array) {
                auto const id = static_cast<uint16_t>(js.value("id", 0));
                auto const path = js.value("path", "");
                auto const latency = static_cast<uint8_t>(js.value("latency", 1));
                _meshes.emplace_back(id, std::string(path), latency);
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

                if (auto animations = js.find("animations"); animations != js.end()) {
                    graphics.animations.reserve(animations->size());

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

                        graphics.animations.emplace_back(getAnimId(name), weight, id);
                    }
                }
                _descriptions.emplace(name, std::move(graphics));
            }
        }
    }
}