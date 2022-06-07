#include "Graphics.h"

#include "../Core/AssetManager.h"
#include "Texture/TextureLoader.h"
#include "Mesh/MeshLoader.h"
#include "Text/FontLoader.h"

#include "../Core/Engine.h"
#include "Vulkan/VkRenderer.h"
#include "GpuProgramsManager.h"
#include "Text/Font.h"
#include "Render/RenderHelper.h"
#include "Animation/AnimationManager.h"

#include "../Log/Log.h"

#include <Platform_inc.h>

#include <cstdint>
#include <string>

namespace engine {

	Graphics::Graphics(const GraphicConfig& cfg) : _config(cfg) {
		auto&& device = Engine::getInstance().getModule<Device>();
		const IRenderSurfaceInitialiser* surfaceInitialiser = device->getSurfaceInitialiser();

		{
			using namespace vulkan;
	
			VkPhysicalDeviceFeatures enabledFeatures{};
			std::vector<const char*> enabledDeviceExtensions; 
			
			VulkanRenderer* renderer = new VulkanRenderer();
			renderer->createInstance({}, {});

			//enabledFeatures.fillModeNonSolid = 1; // example to enable POLYGON_MODE_LINE or POLYGON_MODE_POINT

			renderer->createDevice(enabledFeatures, enabledDeviceExtensions, static_cast<VkPhysicalDeviceType>(_config.gpu_type));
			renderer->createSwapChain(surfaceInitialiser, _config.v_sync);
			renderer->init();

			{ // print gpu info
				std::string gpuTypes[5] = {
					"device_type_other",
					"device_type_integrated_gpu",
					"device_type_discrete_gpu",
					"device_type_virtual_gpu",
					"device_type_cpu"
				};

				const auto& gpuProperties = renderer->getDevice()->gpuProperties;
				LOG_TAG(GRAPHICS, "gpu: %s(%s, driver: %d)", gpuProperties.deviceName, gpuTypes[static_cast<uint8_t>(gpuProperties.deviceType)].c_str(), gpuProperties.driverVersion);
			}

			//render_type = Render_Type::VULKAN;
			_renderer = renderer;
		}

		_gpuProgramManager = new GpuProgramsManager();
		_fontsManager = new FontsManager();
		_renderHelper = new RenderHelper(static_cast<render_type*>(_renderer), _gpuProgramManager);
		_animationManager = new AnimationManager();

		createLoaders();
	}

	Graphics::~Graphics() {
		delete _renderHelper;
		delete _fontsManager;
		delete _gpuProgramManager;
		delete _animationManager;
		delete static_cast<render_type*>(_renderer);
	}

	void Graphics::createLoaders() {
		Engine::getInstance().getModule<AssetManager>()->setLoader<TextureLoader>();
		Engine::getInstance().getModule<AssetManager>()->setLoader<MeshLoader>();
		Engine::getInstance().getModule<AssetManager>()->setLoader<FontLoader>();
	}

	void Graphics::onEngineInitComplete() {
		_renderHelper->initCommonPipelines();
		//Engine::getInstance().getModule<Device>()->swicthFullscreen(true);
	}

	void Graphics::deviceDestroyed() {
		static_cast<render_type*>(_renderer)->waitWorkComplete();
	}

	void Graphics::resize(const uint16_t w, const uint16_t h) {
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