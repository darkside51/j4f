#pragma once

#include "../Core/EngineModule.h"
#include "../Core/Configs.h"
#include <cstdint>

namespace vulkan {
	class VulkanRenderer;
}

namespace engine {

	class GpuProgramsManager;
	class FontsManager;
	class RenderHelper;
	class AnimationManager;

	enum class Render_Type : uint8_t {
		VULKAN = 0
	};

	class Graphics : public IEngineModule {
	public:
		using render_type = vulkan::VulkanRenderer;

		//inline static Render_Type render_type = Render_Type::VULKAN;

		Graphics(const GraphicConfig& cfg);
		~Graphics();

		inline auto getRenderer() const { return static_cast<render_type*>(_renderer); }
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

	private:
		void createLoaders();

		GraphicConfig _config;

		void* _renderer;
		GpuProgramsManager* _gpuProgramManager;
		FontsManager* _fontsManager;
		RenderHelper* _renderHelper;
		AnimationManager* _animationManager;
	};

}