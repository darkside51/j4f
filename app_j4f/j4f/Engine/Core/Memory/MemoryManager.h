#pragma once

#include "ChunkedMemoryPool.h"

#include "../Common.h"
#include "../EngineModule.h"
#include "../Hash.h"
#include <unordered_map>

namespace engine {

	struct IMemoryPool {
		virtual ~IMemoryPool() = default;
		virtual inline void destroyObject(void* object) = 0;
	};

	template <typename T>
	struct TMemoryPool : public IMemoryPool {
		ChunkedMemoryPool<T> pool;

		TMemoryPool(const size_t sz) : pool(sz) {}

		inline void destroyObject(void* object) override {
			pool.destroyObject(static_cast<T*>(object));
		}
	};

	class MemoryManager : public IEngineModule {
	public:
		~MemoryManager() {
			for (auto it = _pools.begin(); it != _pools.end(); ++it) {
				delete it->second;
			}
			_pools.clear();
		}

		template <typename T>
		inline T* allocOnStack(const size_t size) const { return static_cast<T*>(alloca(size)); }

		template <typename T>
		inline T* allocOnHeap(const size_t size) const { return static_cast<T*>(malloc(size)); }

		inline void freeMemory(void* m) const { free(m); }

		template <typename T, typename... Args>
		inline T* createFromPool(Args&&... args) {
			constexpr uint16_t type_id = getUniqueId<T>();
			TMemoryPool<T>* pool;

			auto it = _pools.find(type_id);
			if (it != _pools.end()) {
				pool = it->second;
			} else {
				pool = new TMemoryPool<T>(128);
				_pools.insert(type_id, pool);
			}

			return pool->pool.createObject(std::forward<Args>(args)...);
		}

		template <typename T>
		inline void destroyObjectStrict(T* object) { // если тип точный
			constexpr uint16_t type_id = getUniqueId<T>();
			auto it = _pools.find(type_id);
			if (it != _pools.end()) {
				static_cast<TMemoryPool<T>*>(it->second)->destroyObject(object);
			}
		}

		template <typename T>
		inline void destroyObjectVirtual(T* object) { // если у класса есть функция rtti() и тип, возможно наследуемый
			const uint16_t type_id = object->rtti();
			auto it = _pools.find(type_id);
			if (it != _pools.end()) {
				static_cast<TMemoryPool<T>*>(it->second)->destroyObject(object);
			}
		}

	private:
		std::unordered_map<uint16_t, IMemoryPool*, Hasher<uint16_t>> _pools;
	};

}