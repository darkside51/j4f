#pragma once

#include "../Core/EngineModule.h"
#include "../Core/Configs.h"
#include <Platform_inc.h>
#include <cstdint>
#include <utility>

namespace vulkan {
	class VulkanRenderer;
}

namespace engine {

	class GpuProgramsManager;
	class FontsManager;
	class RenderHelper;
	class AnimationManager;

	class Graphics : public IEngineModule {
	public:
		Graphics(const GraphicConfig& cfg);
		~Graphics();

		inline auto getRenderer() const { return _renderer; }
		inline GpuProgramsManager* getGpuProgramsManager() const { return _gpuProgramManager; }
		inline FontsManager* getFontsManager() const { return _fontsManager; }
		inline RenderHelper* getRenderHelper() const { return _renderHelper; }
		inline AnimationManager* getAnimationManager() const { return _animationManager; }

		void resize(const uint16_t w, const uint16_t h);

		void beginFrame();
		void endFrame();

		void deviceDestroyed();

		void onEngineInitComplete();

		inline const GraphicConfig& config() const { return _config; }

		inline const std::pair<uint16_t, uint16_t>& getSize() const { return _size; }

	private:
        void createRenderer();
		void createLoaders();
        void createRenderHelper();

		GraphicConfig _config;
		std::pair<uint16_t, uint16_t> _size;

		Renderer* _renderer;
		GpuProgramsManager* _gpuProgramManager;
		FontsManager* _fontsManager;
		RenderHelper* _renderHelper;
		AnimationManager* _animationManager;
	};

}