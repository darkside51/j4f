#pragma once

#include <cstdint>

namespace engine {
	struct GraphicConfig {
		bool v_sync;
		uint8_t gpu_type;
	};

	enum class FpsLimitType : uint8_t {
		F_DONT_CARE = 0,
		F_STRICT = 1,
		F_CPU_SLEEP =21
	};

	struct EngineConfig {
		FpsLimitType fpsLimitType;
		uint16_t fpsLimit;
		GraphicConfig graphicsCfg;
	};
}