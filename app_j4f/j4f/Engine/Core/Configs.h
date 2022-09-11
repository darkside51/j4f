#pragma once

#include <cstdint>

namespace engine {
	struct GraphicConfig {
		bool v_sync;
		uint8_t gpu_type;
	};

	struct EngineConfig {
		uint16_t fpsLimit;
		GraphicConfig graphicsCfg;
	};
}