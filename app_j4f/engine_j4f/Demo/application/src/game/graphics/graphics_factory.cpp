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
        for (auto & js: json["gpu_programs"]) {
            auto name = js.value("name", "");
            GPUProgramDescription description;
            for (auto & sh : js["programs"]) {
                auto type = sh.value("type", 0);
                auto path = sh.value("path", "");
                description.modules[type] = path;
            }
            _gpuPrograms.emplace(name, description);
        }
    }
}