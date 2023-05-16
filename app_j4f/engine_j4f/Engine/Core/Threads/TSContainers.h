#pragma once

#include "Synchronisations.h"
#include <unordered_map>

namespace engine {

	inline static const void* m_null_pointer = nullptr;

	template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>>
	class TsUnorderedMap {
	public:

		~TsUnorderedMap() {
			AtomicLock lock(_writer);

			if constexpr (std::is_pointer<V>::value) {
				for (auto it = _map.begin(); it != _map.end(); ++it) {
					delete it->second;
				}
			}

			_map.clear();
		}

		template <typename KEY = K> // KEY may be const K& there
		inline const V& getValue(KEY&& key) {
			AtomicLock::wait(_writer);

			_readers.add();
			auto it = _map.find(key);
			if (it != _map.end()) {
				const V& value = it->second;
				_readers.sub();
				return value;
			}

			_readers.sub();
			if constexpr (std::is_pointer_v<V> || is_smart_pointer_v<V>) {
				return (const V&)(m_null_pointer); // c - style cast, how convert this with c++ cast?
			} else {
				static const V empty(0);
				return empty;
			}
		}

		template <typename KEY = K, typename VAL = V>
		const V& setValue(KEY&& key, VAL&& val) {
			AtomicLock lock(_writer);
			_readers.waitForEmpty();
			_map[key] = val;
			return _map[key];
		}

		template <typename KEY = K, typename VAL = V>
		const V& getOrSet(KEY&& key, VAL&& val) {
			AtomicLock lock(_writer);

			auto it = _map.find(key);
			if (it != _map.end()) {
				return it->second;
			}

			_readers.waitForEmpty();
			_map[key] = val;
			return _map[key];
		}

		template <typename KEY = K, typename F, typename ...Args>
		const V& getOrCreate(KEY&& key, F&& f, Args&&... args) {
			AtomicLock lock(_writer);

			auto it = _map.find(key);
			if (it != _map.end()) {
				return it->second;
			}

			_readers.waitForEmpty();
			_map[key] = f(std::forward<Args>(args)...);
			return _map[key];
		}

		template <typename KEY = K, typename FC, typename F, typename ...Args>
		const V& getOrCreateWithCallback(KEY&& key, FC&& callback, F&& f, Args&&... args) {
			AtomicLock lock(_writer);

			auto it = _map.find(key);
			if (it != _map.end()) {
				callback(it -> second);
				return it->second;
			}

			_readers.waitForEmpty();
			_map[key] = f(std::forward<Args>(args)...);
			callback(_map[key]);
			return _map[key];
		}

		template <typename KEY = K>
		bool hasValue(KEY&& key) {
			AtomicLock::wait(_writer);
			_readers.add();
			auto it = _map.find(key);
			if (it != _map.end()) {
				_readers.sub();
				return true;
			}

			_readers.sub();
			return false;
		}

		template <typename KEY = K>
		inline void erase(KEY&& key) {
			AtomicLock::wait(_writer);
			_readers.add();

			auto it = _map.find(key);
			if (it != _map.end()) {
				_readers.sub();

				AtomicLock lock(_writer);
				_readers.waitForEmpty();

				if constexpr (std::is_pointer<V>::value) {
					delete it->second;
				}
				_map.erase(it);
			} else {
				_readers.sub();
			}
		}

	private:
		std::atomic_bool _writer;
		AtomicCounter _readers;
		std::unordered_map<K, V, H, E> _map;
	};

}