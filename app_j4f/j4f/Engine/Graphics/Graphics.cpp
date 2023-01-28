#include "Graphics.h"

#include "../Core/AssetManager.h"
#include "Texture/TextureLoader.h"
#include "Mesh/MeshLoader.h"
#include "Text/FontLoader.h"

#include "../Core/Engine.h"
#include "Vulkan/vkRenderer.h"
#include "GpuProgramsManager.h"
#include "Text/Font.h"
#include "Render/RenderHelper.h"
#include "Scene/Shadows/CascadeShadowMap.h"
#include "Animation/AnimationManager.h"

#include "../Log/Log.h"

#include <Platform_inc.h>

#include <cstdint>
#include <string>

namespace engine {

	Graphics::Graphics(const GraphicConfig& cfg) :
    _config(cfg),
    _gpuProgramManager(new GpuProgramsManager()),
    _fontsManager(new FontsManager()),
    _animationManager(new AnimationManager())
    {

	}

	Graphics::~Graphics() {
		delete _renderHelper;
		delete _fontsManager;
		delete _gpuProgramManager;
		delete _animationManager;
		delete static_cast<render_type*>(_renderer);
	}

    void Graphics::createRenderer() {
        auto&& device = Engine::getInstance().getModule<Device>();
        const IRenderSurfaceInitialiser* surfaceInitialiser = device->getSurfaceInitialiser();

        {
            using namespace vulkan;

            VkPhysicalDeviceFeatures enabledFeatures{};
            memcpy(&enabledFeatures, &_config.gpu_features, sizeof(VkPhysicalDeviceFeatures));
            std::vector<const char*> &enabledDeviceExtensions = _config.gpu_extensions;

            VulkanRenderer* renderer = new vulkan::VulkanRenderer();
            renderer->createInstance({}, {});

            auto sz = sizeof(VkPhysicalDeviceFeatures);
            auto sz2 = sizeof(GraphicConfig::GPUFeatures);

            renderer->createDevice(enabledFeatures, enabledDeviceExtensions, static_cast<VkPhysicalDeviceType>(_config.gpu_type));
            renderer->createSwapChain(surfaceInitialiser, _config.v_sync);
            renderer->init(_config);

            { // print gpu info
                const char* gpuTypes[5] = {
                        "device_type_other",
                        "device_type_integrated_gpu",
                        "device_type_discrete_gpu",
                        "device_type_virtual_gpu",
                        "device_type_cpu"
                };

                const auto& gpuProperties = renderer->getDevice()->gpuProperties;
                //LOG_TAG(GRAPHICS, "gpu: {}({}, driver: {})", gpuProperties.deviceName, gpuTypes[static_cast<uint8_t>(gpuProperties.deviceType)].c_str(), gpuProperties.driverVersion);
                LOG_TAG(GRAPHICS, "gpu: %s(%s, driver: %d)", gpuProperties.deviceName, gpuTypes[static_cast<uint8_t>(gpuProperties.deviceType)], gpuProperties.driverVersion);
            }

            //render_type = Render_Type::VULKAN;
            const uint64_t wh = renderer->getWH();
            _size.first = static_cast<uint16_t>(wh >> 0);
            _size.second = static_cast<uint16_t>(wh >> 32);

            _renderer = renderer;
        }
    }

	void Graphics::createLoaders() {
		Engine::getInstance().getModule<AssetManager>()->setLoader<TextureLoader>();
		Engine::getInstance().getModule<AssetManager>()->setLoader<MeshLoader>();
		Engine::getInstance().getModule<AssetManager>()->setLoader<FontLoader>();
	}

    void Graphics::createRenderHelper() {
        _renderHelper = new RenderHelper(static_cast<render_type*>(_renderer), _gpuProgramManager);
        _renderHelper->initCommonPipelines();
    }

	void Graphics::onEngineInitComplete() {
        createRenderer();
        createLoaders();
        createRenderHelper();
		CascadeShadowMap::initCommonData();
		//Engine::getInstance().getModule<Device>()->swicthFullscreen(true);
	}

	void Graphics::deviceDestroyed() {
		static_cast<render_type*>(_renderer)->waitWorkComplete();
	}

	void Graphics::resize(const uint16_t w, const uint16_t h) {
		_size.first = w; _size.second = h;
		static_cast<render_type*>(_renderer)->resize(w, h, _config.v_sync);
		_renderHelper->onResize();
	}

	void Graphics::beginFrame() {
		render_type* renderer = static_cast<render_type*>(_renderer);
		renderer->beginFrame();
		_renderHelper->updateFrame();
	}

	void Graphics::endFrame() {
		render_type* renderer = static_cast<render_type*>(_renderer);
		renderer->endFrame();
	}
}