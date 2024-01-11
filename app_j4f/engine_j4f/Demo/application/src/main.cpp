#include <Engine/Core/Engine.h>

int main() {
    engine::EngineConfig config;
    config.fpsLimitDraw = engine::FpsLimit(120u, engine::FpsLimitType::F_CPU_SLEEP);
    config.fpsLimitUpdate = engine::FpsLimit(60u, engine::FpsLimitType::F_CPU_SLEEP);
    config.graphicsCfg = {engine::GpuType::DISCRETE, true, false, engine::Version(1, 0, 0)};
    engine::Engine::getInstance().init(config);
    return 1;
}