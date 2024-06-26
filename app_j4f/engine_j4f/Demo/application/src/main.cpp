#include <Engine/Core/Engine.h>

int main() {
    engine::EngineConfig config;
    config.fpsLimitDraw = engine::FpsLimit(120u, engine::FpsLimitType::F_CPU_SLEEP);
    config.fpsLimitUpdate = engine::FpsLimit(60u, engine::FpsLimitType::F_CPU_SLEEP);
    config.graphicsCfg = {
        engine::GpuType::DISCRETE,  // preffered gpu_type
        true,                       // v_sync
        false,                      // can_continue_main_render_pass
        true,                       // use_stencil_buffer
        engine::Version(1u, 0u, 0u) // render_api_version
    };
    config.graphicsCfg.gpu_features.wideLines = 1u;
	config.graphicsCfg.gpu_features.fillModeNonSolid = 1u; // example to enable POLYGON_MODE_LINE or POLYGON_MODE_POINT

    engine::Engine::getInstance().init(config);
    return 1;
}