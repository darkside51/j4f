#pragma once

#include "../../Core/AssetManager.h"
#include "../../Core/Threads/ThreadPool.h"

#include "../Vulkan/vkBuffer.h"

#include "Loader_gltf.h"

#include <string>
#include <vector>
#include <atomic>
#include <memory>

namespace engine {

	class Mesh;
	struct Mesh_Data;

	inline void semanticsMask(uint16_t& mask, gltf::AttributesSemantic s) {
		mask |= 1 << static_cast<uint16_t>(s);
	}

	template <typename... Args>
	inline void semanticsMask(uint16_t& mask, gltf::AttributesSemantic s, Args&&... args) {
		mask |= 1 << static_cast<uint16_t>(s);
		semanticsMask(mask, std::forward<Args>(args)...);
	}

	template <typename... Args>
	inline uint16_t makeSemanticsMask(Args&&... args) {
		uint16_t mask = 0;
		semanticsMask(mask, std::forward<Args>(args)...);
		return mask;
	}

	struct MeshGraphicsDataBuffer {

		MeshGraphicsDataBuffer() : vb(nullptr), ib(nullptr), vbOffset(0), ibOffset(0) { }
		MeshGraphicsDataBuffer(const size_t vbSize, const size_t ibSize);

		~MeshGraphicsDataBuffer() {
			if (vb) {
				delete vb;
				vb = nullptr;
			}

			if (ib) {
				delete ib;
				ib = nullptr;
			}
		}

		vulkan::VulkanBuffer* vb;
		vulkan::VulkanBuffer* ib;
		VkDeviceSize vbOffset;
		VkDeviceSize ibOffset;
	};

	//////// mesh loader
	template<>
	struct AssetLoadingParams<Mesh> : public AssetLoadingFlags {
		std::string file;
		uint16_t semanticMask = 0u;
		uint8_t latency = 1u;
		uint8_t callbackThreadId = 0u;
		MeshGraphicsDataBuffer* graphicsBuffer = nullptr;
		bool useOffsetsInRenderData = false; // parameter used with none zero vbOffset or ibOffset for fill correct renderData values
	};

	using MeshLoadingParams = AssetLoadingParams<Mesh>;
	using MeshLoadingCallback = AssetLoadingCallback<Mesh*>;
    using MeshPtrLoadingCallback = AssetLoadingCallback<std::unique_ptr<Mesh>>;

	class MeshLoader {
	public:
		using asset_type = Mesh*;
		static void loadAsset(Mesh*& v, const MeshLoadingParams& params, const MeshLoadingCallback& callback);
        static void cleanUp() noexcept;
	private:
		struct DataLoadingCallback {
            std::unique_ptr<Mesh> mesh;
			uint16_t semanticMask;
			uint8_t latency;
			uint8_t targetThreadId = 0u;
			MeshLoadingCallback callback;

            ~DataLoadingCallback();

            DataLoadingCallback(std::unique_ptr<Mesh>&& m, const MeshLoadingCallback& c, uint16_t msk, uint8_t l, uint8_t t);
            DataLoadingCallback(DataLoadingCallback&& other) noexcept;
		};

        static void addCallback(Mesh_Data*, std::unique_ptr<Mesh>&& mesh, const MeshLoadingCallback&, uint16_t mask, uint8_t l, uint8_t thread);
		static void executeCallbacks(Mesh_Data*, const AssetLoadingResult);

		static void fillMeshData(Mesh_Data*, const MeshLoadingParams&);

		inline static std::atomic_bool _graphicsBuffersOffsetsLock;
		inline static std::atomic_bool _callbacksLock;
		inline static std::unordered_map<Mesh_Data*, std::vector<DataLoadingCallback>> _callbacks;
	};
}