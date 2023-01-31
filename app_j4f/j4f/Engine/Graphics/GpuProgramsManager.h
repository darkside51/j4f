#pragma once

#include <string>
#include <vector>

namespace vulkan {
    class VulkanGpuProgram;
}

namespace engine {

	using GpuProgram = vulkan::VulkanGpuProgram;

	enum class ProgramStage {
        VERTEX                  = 0x00000001,
        TESSELLATION_CONTROL    = 0x00000002,
        TESSELLATION_EVALUATION = 0x00000004,
        GEOMETRY                = 0x00000008,
        FRAGMENT                = 0x00000010,
        COMPUTE                 = 0x00000020,
        RAYGEN                  = 0x00000100,
        ANY_HIT                 = 0x00000200,
        CLOSEST_HIT             = 0x00000400,
        MISS                    = 0x00000800,
        INTERSECTION            = 0x00001000,
        CALLABLE                = 0x00002000,
        TASK                    = 0x00000040,
        MESH                    = 0x00000080
	};

    struct ProgramStageInfo {
        ProgramStage stage;
        std::string pass;
        void* specialization;
        ProgramStageInfo(const ProgramStage s, const std::string& p, void* sp = nullptr) : stage(s), pass(p), specialization(sp) {}
        ProgramStageInfo(const ProgramStage s, std::string&& p, void* sp = nullptr) : stage(s), pass(std::move(p)), specialization(sp) {}

        inline bool operator == (const ProgramStageInfo& r) const noexcept {
            return stage == r.stage && pass == r.pass && specialization == r.specialization;
        }
    };

	class GpuProgramsManager {
	public:
        [[nodiscard]] GpuProgram* getProgram(const std::vector<ProgramStageInfo>& stages) const;
	private:
	};

}