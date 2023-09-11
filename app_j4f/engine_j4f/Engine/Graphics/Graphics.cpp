#include "Graphics.h"

#include "../Core/AssetManager.h"
#include "Texture/TextureLoader.h"
#include "Texture/TexturePtrLoader.h"
#include "Mesh/MeshLoader.h"
#include "Text/FontLoader.h"

#include "../Core/Engine.h"
#include "Vulkan/vkRenderer.h"
#include "GpuProgramsManager.h"
#include "Text/Font.h"
#include "Render/RenderHelper.h"
#include "Features/Shadows/CascadeShadowMap.h"
#include "Animation/AnimationManager.h"
#include "Texture/TextureCache.h"

#include <cstdint>

namespace engine {

    Graphics::Graphics(const GraphicConfig &cfg) :
            _config(cfg),
            _renderer(new Renderer()),
            _gpuProgramManager(new GpuProgramsManager()),
            _fontsManager(new FontsManager()),
            _animationManager(new AnimationManager()) {
    }

    Graphics::~Graphics() {
        delete _renderHelper;
        delete _fontsManager;
        delete _gpuProgramManager;
        delete _animationManager;
        delete _renderer;
    }

    void Graphics::createRenderer() {
        if (!_renderer) {
            return;
        }

        auto &&device = Engine::getInstance().getModule<Device>();
        const IRenderSurfaceInitializer *surfaceInitializer = device.getSurfaceInitializer();

        {
            using namespace vulkan;

            VkPhysicalDeviceFeatures enabledFeatures{};
            memcpy(&enabledFeatures, &_config.gpu_features, sizeof(VkPhysicalDeviceFeatures));
            std::vector<const char *> &enabledDeviceExtensions = _config.gpu_extensions;

            _renderer->createInstance(_config.render_api_version,
                                      Engine::getInstance().version(),
                                      Engine::getInstance().applicationVersion(),
                                      {}, {});
            _renderer->createDevice(enabledFeatures, enabledDeviceExtensions,
                                    static_cast<VkPhysicalDeviceType>(_config.gpu_type));
            _renderer->createSwapChain(surfaceInitializer, _config.v_sync);
            _renderer->init(_config);

            { // print gpu info
                const char *gpuTypes[5] = {
                        "device_type_other",
                        "device_type_integrated_gpu",
                        "device_type_discrete_gpu",
                        "device_type_virtual_gpu",
                        "device_type_cpu"
                };

                const auto &gpuProperties = _renderer->getDevice()->gpuProperties;
                //LOG_TAG(GRAPHICS, "gpu: {}({}, driver: {})", gpuProperties.deviceName, gpuTypes[static_cast<uint8_t>(gpuProperties.deviceType)].c_str(), gpuProperties.driverVersion);
                LOG_TAG(GRAPHICS, "gpu: %s(%s, driver: %d)", gpuProperties.deviceName,
                        gpuTypes[static_cast<uint8_t>(gpuProperties.deviceType)], gpuProperties.driverVersion);
            }

            //render_type = Render_Type::VULKAN;
            const auto [w, h] = _renderer->getSize();
            _size.first = static_cast<uint16_t>(w);
            _size.second = static_cast<uint16_t>(h);
        }
    }

    void Graphics::createLoaders() {
        // create texture cache
        Engine::getInstance().getModule<CacheManager>()
                .emplaceCache<TextureCache::value_type, TextureCache::key_type>(
                        std::make_unique<TextureCache>()
                                );

        auto && assetManager = Engine::getInstance().getModule<AssetManager>();

        assetManager.setLoader<TexturePtrLoader>();
        assetManager.setLoader<TextureLoader>();
        assetManager.setLoader<MeshLoader>();
        assetManager.setLoader<FontLoader>();
    }

    void Graphics::createRenderHelper() {
        _renderHelper = new RenderHelper(_renderer, _gpuProgramManager);
        _renderHelper->initCommonPipelines();
    }

    void Graphics::onEngineInitComplete() {
        createRenderer();
        createLoaders();
        createRenderHelper();
        initFeatures();
        //Engine::getInstance().getModule<Device>()->swicthFullscreen(true);
    }

    void Graphics::initFeatures() {
        _features.initFeatures();
    }

    void Graphics::deviceDestroyed() {
        if (_renderer) {
            _renderer->waitWorkComplete();
        }
    }

    void Graphics::resize(const uint16_t w, const uint16_t h) {
        _size.first = w;
        _size.second = h;

        if (_renderer) {
            _renderer->resize(w, h, _config.v_sync);
        }

        if (_renderHelper) {
            _renderHelper->onResize();
        }
    }

    void Graphics::beginFrame() {
        if (_renderer) {
            _renderer->beginFrame();
        }

        if (_renderHelper) {
            _renderHelper->updateFrame();
        }
    }

    void Graphics::endFrame() {
        if (_renderer) {
            _renderer->endFrame();
        }
    }
}