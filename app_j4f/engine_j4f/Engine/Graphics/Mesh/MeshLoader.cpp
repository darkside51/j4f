#include "MeshLoader.h"
#include "../../Core/Engine.h"
#include "../../Core/Cache.h"
#include "../../Core/Threads/WorkersCommutator.h"
#include "../../Core/FakeCopyable.h"
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

    MeshLoader::DataLoadingCallback::DataLoadingCallback(std::unique_ptr<Mesh>&& m, const MeshLoadingCallback& c, uint16_t msk, uint8_t l, uint8_t t) :
    mesh(std::move(m)), callback(c), semanticMask(msk), latency(l), targetThreadId(t) { }

    MeshLoader::DataLoadingCallback::DataLoadingCallback(DataLoadingCallback&& other) noexcept :
    mesh(std::move(other.mesh)),
    semanticMask(other.semanticMask),
    latency(other.latency),
    targetThreadId(other.targetThreadId),
    callback(std::move(other.callback)) { }

    MeshLoader::DataLoadingCallback::~DataLoadingCallback() = default;

    void MeshLoader::addCallback(Mesh_Data* md, std::unique_ptr<Mesh>&& mesh, const MeshLoadingCallback& c, uint16_t mask, uint8_t l, uint8_t thread) {
        AtomicLock lock(_callbacksLock);
        _callbacks[md].emplace_back(std::move(mesh), c, mask, l, thread);
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

            CopyWrapper execute([callback = std::move(c.callback), mesh = std::move(c.mesh)]() mutable {
                if (callback) {
                    callback(mesh.release(), AssetLoadingResult::LOADING_SUCCESS);
                }
            });

            threadCommutator.enqueue(c.targetThreadId,
                                     [execute = std::move(execute)](const CancellationToken &) mutable {
                                         execute();
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

		for (uint8_t i = 0u; i < 16u; ++i) {
			if (params.semanticMask & (1u << i)) {
				allowedAttributes.emplace_back(static_cast<AttributesSemantic>(i));
			}
		}

		VkDeviceSize vbOffset;
		VkDeviceSize ibOffset;

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

//        threadCommutator.enqueue(engine.getThreadCommutationId(Engine::Workers::UPDATE_THREAD),
//                                 [mData](const CancellationToken &){ mData->fillGpuData(); });

		executeCallbacks(mData, AssetLoadingResult::LOADING_SUCCESS);
	}

	void MeshLoader::loadAsset(Mesh*& v, const MeshLoadingParams& params, const MeshLoadingCallback& callback) {
        auto mesh = std::make_unique<Mesh>();
        v = mesh.get();
		
		auto&& engine = Engine::getInstance();
		auto&& meshDataCache = engine.getModule<CacheManager>().getCache<std::string, Mesh_Data*>();

		if (Mesh_Data* mData = meshDataCache->getValue(params.file)) {			
			if (mData->indicesBuffer && mData->verticesBuffer) {
				v->createWithData(mData, params.semanticMask, params.latency);
				if (callback) {
                    callback(mesh.release(), AssetLoadingResult::LOADING_SUCCESS);
                }
			} else {
				addCallback(mData, std::move(mesh), callback, params.semanticMask, params.latency, params.callbackThreadId);
			}
			return;
		}

		meshDataCache->getOrSetValue(params.file, [](
                std::unique_ptr<Mesh>&& v,
                const MeshLoadingParams& params,
                const MeshLoadingCallback& callback
                ) {
			auto* mData = new Mesh_Data();
			addCallback(mData, std::move(v), callback, params.semanticMask, params.latency, params.callbackThreadId);

			if (params.flags->async) {
                Engine::getInstance().getModule<AssetManager>().getThreadPool()->enqueue(
                        TaskType::COMMON,
                         [](const CancellationToken& token,
                               const MeshLoadingParams params, Mesh_Data* mData) {
					fillMeshData(mData, params);
				}, params, mData);
			} else {
				fillMeshData(mData, params);
			}

			return mData;
		}, std::move(mesh), params, callback);
	}

    void MeshLoader::cleanUp() noexcept {
        AtomicLock lock(_callbacksLock);
        _callbacks.clear();
    }
}