#pragma once

#include "../Core/EngineModule.h"
#include "../Core/Configs.h"
#include "Features/Feature.h"
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
		explicit Graphics(const GraphicConfig& cfg);
		~Graphics() override;

        [[nodiscard]] inline auto getRenderer() const { return _renderer; }
		[[nodiscard]] inline GpuProgramsManager* getGpuProgramsManager() const { return _gpuProgramManager; }
        [[nodiscard]] inline FontsManager* getFontsManager() const { return _fontsManager; }
        [[nodiscard]] inline RenderHelper* getRenderHelper() const { return _renderHelper; }
        [[nodiscard]] inline AnimationManager* getAnimationManager() const { return _animationManager; }

		void resize(const uint16_t w, const uint16_t h);

		void beginFrame();
		void endFrame();

		void deviceDestroyed();

		void onEngineInitComplete();

        [[nodiscard]] inline const GraphicConfig& config() const noexcept { return _config; }

        [[nodiscard]] inline const std::pair<uint16_t, uint16_t>& getSize() const noexcept { return _size; }

        Features& features() & noexcept { return _features; }

	private:
        void createRenderer();
		void createLoaders();
        void createRenderHelper();
        void initFeatures();

		GraphicConfig _config;
		std::pair<uint16_t, uint16_t> _size;

		Renderer* _renderer = nullptr;
		GpuProgramsManager* _gpuProgramManager = nullptr;
		FontsManager* _fontsManager = nullptr;
		RenderHelper* _renderHelper = nullptr;
		AnimationManager* _animationManager = nullptr;
        Features _features;
	};

}