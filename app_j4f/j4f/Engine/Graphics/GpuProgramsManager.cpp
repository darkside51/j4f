#include "GpuProgramsManager.h"
#include "../Core/Engine.h"
#include "../Core/Cache.h"
#include "Graphics.h"
#include "../Core/Common.h"
#include "Vulkan/vkGPUProgram.h"

namespace engine {

	class GpuProgramKey {
	public:
		explicit GpuProgramKey(const std::vector<ProgramStageInfo>& s) : _stages(s) {
			_hash = 0;
			for (auto&& stage : _stages) {
				engine::hash_combo(_hash, stage.stage, stage.pass, stage.specialization);
			}
		}

        [[nodiscard]] inline size_t hash() const noexcept { return _hash; }

		inline bool operator == (const GpuProgramKey& r) const noexcept {
			const size_t sz = _stages.size();
			if (sz != r._stages.size()) return false;
			for (size_t i = 0; i < sz; ++i) {
				if (!(_stages[i] == r._stages[i])) return false;
			}
			return true;
		}

	private: 
		std::vector<ProgramStageInfo> _stages;
		size_t _hash;
	};

	GpuProgram* GpuProgramsManager::getProgram(const std::vector<ProgramStageInfo>& stages) const {
		GpuProgramKey key(stages);

		GpuProgram* value = Engine::getInstance().getModule<CacheManager>()->load<GpuProgram*>(
			key,
			[](const std::vector<ProgramStageInfo>& stages) {
				const size_t sz = stages.size();
				std::vector<vulkan::ShaderStageInfo> shaderStagesInfo(sz);
				for (size_t i = 0; i < sz; ++i) {
					shaderStagesInfo[i].modulePass = stages[i].pass.c_str();
					shaderStagesInfo[i].pipelineStage = static_cast<VkShaderStageFlagBits>(stages[i].stage);
					shaderStagesInfo[i].specializationInfo = static_cast<VkSpecializationInfo*>(stages[i].specialization);
				}

				auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();
				return reinterpret_cast<GpuProgram*>(new vulkan::VulkanGpuProgram(renderer, shaderStagesInfo));
			},
			stages
		);

		return value;
	}
}