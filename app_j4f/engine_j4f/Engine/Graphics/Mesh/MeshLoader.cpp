#include "MeshLoader.h"
#include "../../Core/Engine.h"
#include "../../Core/Cache.h"
#include "../../Core/Threads/WorkersCommutator.h"
#include "../Graphics.h"
#include "MeshData.h"
#include "Mesh.h"
#include "../../Utils/Debug/Profiler.h"

namespace engine {

	MeshGraphicsDataBuffer::MeshGraphicsDataBuffer(const size_t vbSize, const size_t ibSize) : vb(new vulkan::VulkanBuffer()), ib(new vulkan::VulkanBuffer()), vbOffset(0), ibOffset(0) {
		auto&& renderer = Engine::getInstance().getModule<Graphics>().getRenderer();

		renderer->getDevice()->createBuffer(
			VK_SHARING_MODE_EXCLUSIVE,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vb,
			vbSize
		);

		renderer->getDevice()->createBuffer(
			VK_SHARING_MODE_EXCLUSIVE,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			ib,
			ibSize
		);
	}

	void MeshLoader::addCallback(Mesh_Data* md, Mesh* mesh, const MeshLoadingCallback& c, uint16_t mask, uint8_t l, uint8_t thread) {
		AtomicLock lock(_callbacksLock);
		_callbacks[md].emplace_back(mesh, c, mask, l, thread);
	}

	void MeshLoader::executeCallbacks(Mesh_Data* m, const AssetLoadingResult result) {
		std::vector<DataLoadingCallback> callbacks;
		{
			AtomicLock lock(_callbacksLock);
			auto it = _callbacks.find(m);
			if (it != _callbacks.end()) {
				callbacks = std::move(it->second);
				_callbacks.erase(it);
			}
		}

        auto && engine = Engine::getInstance();
        auto && threadCommutator = engine.getModule<WorkerThreadsCommutator>();

		for (auto&& c : callbacks) {
			c.mesh->createWithData(m, c.semanticMask, c.latency);

            threadCommutator.enqueue(c.targetThreadId,
                                      [callback = std::move(c.callback), v = c.mesh](const CancellationToken &){
                if (callback) {
                    callback(v, AssetLoadingResult::LOADING_SUCCESS);
                    removeMesh(v);
                }
            });
		}
	}

	void MeshLoader::fillMeshData(Mesh_Data* mData, const MeshLoadingParams& params) {
		PROFILE_TIME_SCOPED_M(meshDataLoading, params.file)

		using namespace gltf;

		PROFILE_TIME_ENTER_SCOPE_M(meshDataJsonParsing, params.file)
		const Layout layout = Parser::loadModel(params.file);
		PROFILE_TIME_LEAVE_SCOPE(meshDataJsonParsing)

		std::vector<AttributesSemantic> allowedAttributes;

		for (uint8_t i = 0; i < 16; ++i) {
			if (params.semanticMask & (1 << i)) {
				allowedAttributes.emplace_back(static_cast<AttributesSemantic>(i));
			}
		}

		VkDeviceSize vbOffset = 0;
		VkDeviceSize ibOffset = 0;

		{
			AtomicLock lock(_graphicsBuffersOffsetsLock);
			vbOffset = params.graphicsBuffer->vbOffset;
			ibOffset = params.graphicsBuffer->ibOffset;

			const auto vertex_offset = mData->loadMeshes(layout, allowedAttributes, vbOffset, ibOffset, params.useOffsetsInRenderData);

			params.graphicsBuffer->vbOffset += mData->vertexSize * mData->vertexCount * sizeof(float) + vertex_offset;
			params.graphicsBuffer->ibOffset += mData->indexCount * sizeof(uint32_t);
		}

		mData->loadSkins(layout);
		mData->loadAnimations(layout);
		mData->loadNodes(layout);

		mData->uploadGpuData(params.graphicsBuffer->vb, params.graphicsBuffer->ib, vbOffset, ibOffset);

        auto && engine = Engine::getInstance();
        auto && threadCommutator = engine.getModule<WorkerThreadsCommutator>();

        threadCommutator.enqueue(engine.getThreadCommutationId(Engine::Workers::RENDER_THREAD),
                                  [mData](const CancellationToken &){ mData->fillGpuData(); });

		executeCallbacks(mData, AssetLoadingResult::LOADING_SUCCESS);
	}

	void MeshLoader::loadAsset(Mesh*& v, const MeshLoadingParams& params, const MeshLoadingCallback& callback) {
	
		v = createMesh();
		
		auto&& engine = Engine::getInstance();
		auto&& meshDataCache = engine.getModule<CacheManager>().getCache<std::string, Mesh_Data*>();

		if (Mesh_Data* mData = meshDataCache->getValue(params.file)) {			
			if (mData->indicesBuffer && mData->verticesBuffer) {
				v->createWithData(mData, params.semanticMask, params.latency);
				if (callback) {
                    callback(v, AssetLoadingResult::LOADING_SUCCESS);
                    removeMesh(v);
                }
			} else {
				addCallback(mData, v, callback, params.semanticMask, params.latency, params.callbackThreadId);
			}
			return;
		}

		Mesh_Data* mData = meshDataCache->getOrSetValue(params.file, [](Mesh* v, const MeshLoadingParams& params, const MeshLoadingCallback callback) {
			auto&& engine = Engine::getInstance();
			Mesh_Data* mData = new Mesh_Data();

			addCallback(mData, v, callback, params.semanticMask, params.latency, params.callbackThreadId);

			if (params.flags->async) {
				engine.getModule<AssetManager>().getThreadPool()->enqueue(TaskType::COMMON, [](const CancellationToken& token, const MeshLoadingParams params, Mesh_Data* mData) {
					fillMeshData(mData, params);
				}, params, mData);
			} else {
				fillMeshData(mData, params);
			}

			return mData;
		}, v, params, callback);
	}

    Mesh* MeshLoader::createMesh() {
        Mesh* result = nullptr;
        {
            AtomicLock lock(_rawDataLock);
            result = _rawData.emplace_back(new Mesh());
        }
        return result;
    }

    void MeshLoader::removeMesh(Mesh* m) {
        AtomicLock lock(_rawDataLock);
        _rawData.erase(std::remove(_rawData.begin(), _rawData.end(), m), _rawData.end());
    }

    void MeshLoader::cleanUp() noexcept {
        AtomicLock lock(_rawDataLock);
        for (auto && m : _rawData) {
            delete m;
        }
        _rawData.clear();
    }
}