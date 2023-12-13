#pragma once

#include "Worker.h"
#include "../EngineModule.h"
#include "../../Utils/Debug/Assert.h"

#include <atomic>
#include <cstdint>
#include <map>

namespace engine {

    class WorkerThreadsCommutator final : public IEngineModule {
    public:
        ~WorkerThreadsCommutator() override = default;

        uint8_t emplaceWorkerThread(WorkerThread *worker) {
            auto const id = _nextId.fetch_add(1u, std::memory_order_relaxed);
            _workers.emplace(id, worker);
            return id;
        }

        template<class F, typename... Args>
        void enqueue(const uint16_t workerId, F &&f,
                     Args &&... args)  {

            auto it = _workers.find(workerId);

            ENGINE_BREAK_CONDITION_DO(it != _workers.end(), THREADS, "no worker with id", return)

            it->second->emplaceTask([f = std::forward<F>(f), args...]() mutable {
               f(std::forward<Args>(args)...);
            });
        }

        WorkerThread* getWorkerThreadByCommutationId(const uint16_t workerId) const {
            auto it = _workers.find(workerId);
            if (it != _workers.end()) {
                return it->second;
            }

            return nullptr;
        }

    private:
        std::atomic_uint8_t _nextId = {0};
        std::map<uint8_t, WorkerThread *> _workers;
    };

}