#include <Engine/Core/Engine.h>

int main() {
	engine::EngineConfig cfg;
	cfg.fpsLimit = 120;
	cfg.fpsLimitType = engine::FpsLimitType::F_DONT_CARE;
	cfg.graphicsCfg = { engine::GpuType::DISCRETE, true, false, engine::Version(1, 0, 0) }; // INTEGRATED, DISCRETE
	engine::Engine::getInstance().init(cfg);
	return 1;
}