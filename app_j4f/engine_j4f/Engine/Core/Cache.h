#pragma once

#include "EngineModule.h"
#include "Common.h"
#include "Hash.h"
#include "Threads/TSContainers.h"

#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>

namespace engine {
	template <typename T>
	using no_const_no_reference_type = typename std::remove_const<typename std::remove_reference<T>::type>::type;
	//using no_const_no_reference_type = typename std::remove_reference<typename std::remove_const<T>::type>::type;

	class ICache {
	public:
		virtual ~ICache() = default;
	};

	template <typename K, typename V>
	class Cache final : public ICache {
	public:
        using key_type = K;
        using value_type = V;

        Cache() = default;
		~Cache() override = default;

		template <typename KEY = K, typename VAL = V>
		inline void setValue(KEY&& key, VAL&& value) { _map.setValue(key, value); }

		template <typename KEY = K>
		inline bool hasValue(KEY&& key) { return _map.hasValue(key); }

		template <typename KEY = K>
		inline const V& getValue(KEY&& key) { return _map.getValue(key); }

		template <typename KEY = K>
		inline void erase(KEY&& key) { _map.erase(key); }

		template <typename KEY = K, typename VAL = V>
		inline const V& getOrSetValue(KEY&& key, VAL&& value) { return _map.getOrSet(key, value); }

		template <typename KEY = K, typename F, typename ...Args>
		inline const V& getOrSetValue(KEY&& key, F&& f, Args&&... args) {
            return _map.getOrCreate(key, std::forward<F>(f), std::forward<Args>(args)...);
        }

		template <typename KEY = K, typename FC, typename F, typename ...Args>
		inline const V& getOrSetValueWithCallback(KEY&& key, FC&& callback, F&& f, Args&&... args) {
            return _map.getOrCreateWithCallback(
                    key, std::forward<FC>(callback),
                    std::forward<F>(f), std::forward<Args>(args)...
                    );
        }

		template <typename KEY = K, typename F, typename ...Args>
		inline const V& getValueOrCreate(KEY&& key, F&& f, Args&&... args) {
			if (auto& value = _map.getValue(key)) {
				return value;
			} else {
				return _map.getOrCreate(key, std::forward<F>(f), std::forward<Args>(args)...);
			}
		}

	private:
		TsUnorderedMap<key_type, value_type, Hasher<key_type>> _map;
	};

	class CacheManager final : public IEngineModule {
	public:
		~CacheManager() override = default;

		template<typename K, typename V>
		inline bool hasCache() noexcept {
			using key_type = no_const_no_reference_type<K>;
			const uint16_t key = getUniqueIdTypes<key_type, V>();
			return _caches.hasValue(key);
		}

		template<typename K, typename V>
		inline auto getCache() {
			using key_type = no_const_no_reference_type<K>;
			const uint16_t key = getUniqueIdTypes<key_type, V>();

			if (auto&& cache = _caches.getValue(key)) {
				return static_cast<Cache<key_type, V>*>(cache.get());
			} else {
				return static_cast<Cache<key_type, V>*>(_caches.getOrCreate(key, []() {
                    return std::make_unique<Cache<key_type, V>>();
                }).get());
			}
		}

        template<typename T>
        inline T* getCache() {
            using key_type = T::key_type;
            using value_type = T::value_type;

            const uint16_t key = getUniqueIdTypes<key_type, value_type>();
            if (auto&& cache = _caches.getValue(key)) {
                return static_cast<T*>(cache.get());
            }

            return nullptr;
        }

		template<typename V, typename K = std::string>
		inline void store(K&& key, const V& v) {
			using key_type = no_const_no_reference_type<K>;
			auto&& cache = getCache<key_type, V>();
			cache->setValue(std::forward<K>(key), v);
		}

		template<typename V, typename K = std::string>
		inline auto load(K&& key) {
			using key_type = no_const_no_reference_type<K>;
			const auto&& cache = getCache<key_type, V>();
			return cache->getValue(std::forward<K>(key));
		}

		template <typename V, typename K = std::string, typename F, typename ...Args>
		inline auto load(K&& key, F&& f, Args&&... args) {
			using key_type = no_const_no_reference_type<K>;
            auto&& cache = getCache<key_type, V>();
            return cache->getValueOrCreate(std::forward<K>(key), std::forward<F>(f), std::forward<Args>(args)...);
		}

        template<typename V, typename K, typename ...Args>
        inline void createCache(Args&&... args) {
            using key_type = no_const_no_reference_type<K>;
            const uint16_t key = getUniqueIdTypes<key_type, V>();

            if (_caches.getValue(key)) {
                return;
            }

            _caches.setValue(key, std::make_unique<Cache<key_type, V>>(std::forward<Args>(args)...));
        }

        template<typename V, typename K>
        inline void emplaceCache(std::unique_ptr<ICache>&& cache) {
            using key_type = no_const_no_reference_type<K>;
            const uint16_t key = getUniqueIdTypes<key_type, V>();

            if (_caches.getValue(key)) {
                return;
            }

            _caches.setValue(key, std::move(cache));
        }

	private:
		inline static std::atomic_uint16_t staticId;

		template <typename T1, typename T2>
		inline static uint16_t getUniqueIdTypes() noexcept {
			static const uint16_t id = staticId.fetch_add(1, std::memory_order_relaxed);
			return id;
		}

        TsUnorderedMap<uint16_t, std::unique_ptr<ICache>> _caches;
	};
}