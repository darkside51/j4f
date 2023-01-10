#pragma once

#include <vector>
#include <cstdint>

namespace engine {

    enum class GpuType : uint8_t {
        OTHER       = 0,
        INTEGRATED  = 1,
        DISCRETE    = 2,
        VIRTUAL     = 3,
        CPU         = 4
    };

	struct GraphicConfig {
        GpuType gpu_type = GpuType::OTHER;
		bool v_sync = true;
        bool can_continue_main_render_pass = false;

        struct GPUFeatures {
            uint32_t    robustBufferAccess = 0;
            uint32_t    fullDrawIndexUint32 = 0;
            uint32_t    imageCubeArray = 0;
            uint32_t    independentBlend = 0;
            uint32_t    geometryShader = 0;
            uint32_t    tessellationShader = 0;
            uint32_t    sampleRateShading = 0;
            uint32_t    dualSrcBlend = 0;
            uint32_t    logicOp = 0;
            uint32_t    multiDrawIndirect = 0;
            uint32_t    drawIndirectFirstInstance = 0;
            uint32_t    depthClamp = 0;
            uint32_t    depthBiasClamp = 0;
            uint32_t    fillModeNonSolid = 0;
            uint32_t    depthBounds = 0;
            uint32_t    wideLines = 0;
            uint32_t    largePoints = 0;
            uint32_t    alphaToOne = 0;
            uint32_t    multiViewport = 0;
            uint32_t    samplerAnisotropy = 0;
            uint32_t    textureCompressionETC2 = 0;
            uint32_t    textureCompressionASTC_LDR = 0;
            uint32_t    textureCompressionBC = 0;
            uint32_t    occlusionQueryPrecise = 0;
            uint32_t    pipelineStatisticsQuery = 0;
            uint32_t    vertexPipelineStoresAndAtomics = 0;
            uint32_t    fragmentStoresAndAtomics = 0;
            uint32_t    shaderTessellationAndGeometryPointSize = 0;
            uint32_t    shaderImageGatherExtended = 0;
            uint32_t    shaderStorageImageExtendedFormats = 0;
            uint32_t    shaderStorageImageMultisample = 0;
            uint32_t    shaderStorageImageReadWithoutFormat = 0;
            uint32_t    shaderStorageImageWriteWithoutFormat = 0;
            uint32_t    shaderUniformBufferArrayDynamicIndexing = 0;
            uint32_t    shaderSampledImageArrayDynamicIndexing = 0;
            uint32_t    shaderStorageBufferArrayDynamicIndexing = 0;
            uint32_t    shaderStorageImageArrayDynamicIndexing = 0;
            uint32_t    shaderClipDistance = 0;
            uint32_t    shaderCullDistance = 0;
            uint32_t    shaderFloat64 = 0;
            uint32_t    shaderInt64 = 0;
            uint32_t    shaderInt16 = 0;
            uint32_t    shaderResourceResidency = 0;
            uint32_t    shaderResourceMinLod = 0;
            uint32_t    sparseBinding = 0;
            uint32_t    sparseResidencyBuffer = 0;
            uint32_t    sparseResidencyImage2D = 0;
            uint32_t    sparseResidencyImage3D = 0;
            uint32_t    sparseResidency2Samples = 0;
            uint32_t    sparseResidency4Samples = 0;
            uint32_t    sparseResidency8Samples = 0;
            uint32_t    sparseResidency16Samples = 0;
            uint32_t    sparseResidencyAliased = 0;
            uint32_t    variableMultisampleRate = 0;
            uint32_t    inheritedQueries = 0;
        } gpu_features;

        std::vector<const char*> gpu_extensions;
	};

	enum class FpsLimitType : uint8_t {
		F_DONT_CARE = 0,
		F_STRICT = 1,
		F_CPU_SLEEP = 2
	};

	struct EngineConfig {
		FpsLimitType fpsLimitType;
		uint16_t fpsLimit;
		GraphicConfig graphicsCfg;
	};
}