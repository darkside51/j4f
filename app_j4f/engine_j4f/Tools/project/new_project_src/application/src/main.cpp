#include <Engine/Core/Engine.h>

int main() {
	engine::EngineConfig cfg;
	cfg.fpsDraw = 120;
	cfg.fpsLimitTypeDraw = engine::FpsLimitType::F_CPU_SLEEP;
	cfg.graphicsCfg = { engine::GpuType::DISCRETE, true, false, engine::Version(1, 0, 0) };
	engine::Engine::getInstance().init(cfg);
	return 1;
}