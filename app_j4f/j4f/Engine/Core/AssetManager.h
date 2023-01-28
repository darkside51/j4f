#pragma once

#include "Common.h"
#include "EngineModule.h"
#include "Threads/ThreadPool.h"

#include <type_traits>
#include <functional>
#include <memory>
#include <cstdint>
#include <unordered_map>

namespace engine {

	namespace {
		template <typename T>
		struct remove_shared_pointer {
			using type = T;
		};

		template<typename T>
		struct remove_shared_pointer<std::shared_ptr<T>> {
			using type = T;
		};

		template <typename T>
		using raw_type_name = typename std::remove_pointer<typename remove_shared_pointer<typename std::remove_reference<typename std::remove_const<T>::type>::type>::type>::type;
	}

	enum class AssetLoadingResult : uint8_t {
		LOADING_SUCCESS = 0,
		LOADING_ERROR = 1,
		LOADER_NO_EXIST = 2
	};

	template <typename T>
	using AssetLoadingCallback = std::function<void(const T& asset, const AssetLoadingResult result)>;

	struct AssetLoadingFlags {
		struct Flags {
			uint8_t async : 1;
			uint8_t use_cache : 1;
		};

		union LoadingFlags {
			uint8_t mask;
			Flags flags;

			LoadingFlags() : mask(0) {}
			LoadingFlags(const uint8_t f) : mask(f) {}

			Flags* operator->() { return &flags; }
			const Flags* operator->() const { return &flags; }
		};

		virtual ~AssetLoadingFlags() = default;

		LoadingFlags flags;
	};

	template<typename T>
	struct AssetLoadingParams : public AssetLoadingFlags {};

	class IAssetLoader {
	public:
		virtual ~IAssetLoader() = default;
		virtual void loadAsset(const AssetLoadingFlags& p, void* data, const void* loadingCallback) const = 0;
	};

	template <typename Loader>
	class AssetLoaderT : public IAssetLoader {
	public:
		void loadAsset(const AssetLoadingFlags& p, void* data, const void* loadingCallback) const override {
			// some magic ;)
			using type = typename Loader::asset_type;
			using params_type = AssetLoadingParams<raw_type_name<type>>;
			type& value = *static_cast<type*>(data);
			const params_type& params = static_cast<const params_type&>(p);
			const AssetLoadingCallback<type>& callback = *(static_cast<const AssetLoadingCallback<type>*>(loadingCallback));
			Loader::loadAsset(value, params, callback);
		}
	};

	class AssetManager : public IEngineModule {
		/*
		// SFINAE конечно, но работает), по другому разрулил, пусть пока полежит тут
		template<typename Loader, typename T, typename ALP = typename AssetLoadingParams<typename raw_type_name<T>>, typename = void>
		struct HasLoadImpl : std::false_type {};

		template<typename Loader, typename T, typename ALP>
		struct HasLoadImpl<Loader, T, ALP, std::enable_if_t<std::is_same<decltype(std::declval<Loader>().loadAssetImpl(ALP())), T>::value>> : std::true_type {};
		*/

	public:
		AssetManager(const uint8_t loaderThreadsCount) : _loaderPool(new ThreadPool(loaderThreadsCount)) {
		}

		~AssetManager() {
			for (auto it = _loaders.begin(); it != _loaders.end(); ++it) {
				delete it->second;
			}
			_loaders.clear();

			_loaderPool->stop();
			delete _loaderPool;
		}

		template<typename T>
		inline bool hasLoader() const {
			const uint16_t loaderId = UniqueTypeId<IAssetLoader>::getUniqueId<T>();
			return (_loaders.find(loaderId) != _loaders.end());
		}

		template<typename T>
		inline const IAssetLoader* getLoader() const {
			const uint16_t loaderId = UniqueTypeId<IAssetLoader>::getUniqueId<T>();
			auto it = _loaders.find(loaderId);
			if (it != _loaders.end()) {
				return it->second;
			}

			return nullptr;
		}

		template<typename Loader>
		inline void setLoader() {
			if (const IAssetLoader* loader = getLoader<typename Loader::asset_type>()) {
				delete loader;
			}
			const uint16_t loaderId = UniqueTypeId<IAssetLoader>::getUniqueId<typename Loader::asset_type>();
			_loaders[loaderId] = new AssetLoaderT<Loader>();
		}

		template<typename Loader>
		inline const IAssetLoader* replaceLoader() { // return old loader if it exist
			const IAssetLoader* loader = getLoader<typename Loader::asset_type>();
			const uint16_t loaderId = UniqueTypeId<IAssetLoader>::getUniqueId<typename Loader::asset_type>();
			_loaders[loaderId] = new AssetLoaderT<Loader>();
			return loader;
		}

		template<typename T, typename ALP = AssetLoadingParams<raw_type_name<T>>, typename Callback = AssetLoadingCallback<T>>
		inline T loadAsset(ALP&& params, Callback&& callback = nullptr) const {
			if (const IAssetLoader* loader = getLoader<T>()) {
				T value;
				const AssetLoadingCallback<T>& f = callback;
				loader->loadAsset(std::forward<ALP>(params), &value, &f);
				return value;
			}

			T value(0);
			if (const AssetLoadingCallback<T>& c = callback) {
				c(value, AssetLoadingResult::LOADER_NO_EXIST);
			}
			return value;
		}

		ThreadPool* getThreadPool() { return _loaderPool; }
		const ThreadPool* getThreadPool() const { return _loaderPool; }

		void deviceDestroyed() {
			_loaderPool->stop();
		}

	private:
		ThreadPool* _loaderPool;
		std::unordered_map<uint16_t, IAssetLoader*> _loaders;
	};

/*
// using:
struct TypeLoader {
	using asset_type = type_you_want_to_load;
	static void loadAsset(type& v, const AssetLoadingParams<raw_type_name<asset_type>>& params, const AssetLoadingCallback<asset_type>& callback) {
		...
		v = ...; // set value to v
		if (callback) {
			callback(v, AssetLoadingResult::LOADING_SUCCESS);
		}
		...
	}
};

// example:
struct IntLoader {
	using asset_type = int;
	static void loadAsset(int& v, const AssetLoadingParams<int>& params, const AssetLoadingCallback<int>& callback) {
		v = 22;
	}
};

struct IntPtrLoader {
	using asset_type = int*;
	static void loadAsset(type& v, const AssetLoadingParams<int>& params, const AssetLoadingCallback<int*>& callback) {
		v = new int(111);
		if (callback) {
			callback(v, AssetLoadingResult::LOADING_SUCCESS);
		}
	}
};

struct IntSharedPtrLoader {
	using asset_type = std::shared_ptr<int>;
	static void loadAsset(std::shared_ptr<int>& v, const AssetLoadingParams<int>& params, const AssetLoadingCallback<std::shared_ptr<int>>& callback) {
		v = std::make_shared<int>(222);
	}
};

// for create loader:
Engine::getInstance().getModule<AssetManager>()->setLoader<IntPtrLoader>();
// or
auto oldLoader = Engine::getInstance().getModule<AssetManager>()->replaceLoader<IntSharedPtrLoader>();
if (oldLoader) {
... some with oldLoader
}
*/

	/*
	////// no template variant
	class ILoadParams {
	public:
		virtual ~ILoadParams() = default;
		virtual size_t loaderId() const = 0;
	};

	struct IAssetData {
		virtual ~IAssetData() = default;
	};

	struct IAssetLoader2 {
		virtual ~IAssetLoader2() = default;
		virtual size_t loaderId() const = 0;
		virtual IAssetData* loadAsset(const ILoadParams& params) const = 0;
	};

	class AssetDataHandler {
	public:
		AssetDataHandler(IAssetData* data) : _data(data) { }
		~AssetDataHandler() {
			if (_data) {
				delete _data;
				_data = nullptr;
			}
		}

		AssetDataHandler(AssetDataHandler&& h) noexcept : _data(std::move(h._data))  {
			h._data = nullptr;
		}

		const AssetDataHandler& operator= (AssetDataHandler&& h) noexcept {
			_data = std::move(h._data);
			h._data = nullptr;
			return *this;
		}

		AssetDataHandler(const AssetDataHandler& h) = delete;
		const AssetDataHandler& operator= (const AssetDataHandler& h) = delete;

		const IAssetData* get() const { return _data; }
		IAssetData* get() { return _data; }

		const IAssetData* operator->() const { return _data; }
		IAssetData* operator->() { return _data; }

	private:
		IAssetData* _data = nullptr;
	};

	class AssetManager2 : public IEngineModule {
	public:
		inline const IAssetLoader2* getLoader(const size_t id) const {
			auto it = _loaders.find(id);
			if (it != _loaders.end()) {
				return it->second;
			}

			return nullptr;
		}

		AssetDataHandler loadAsset(const ILoadParams& params) const {
			if (const IAssetLoader2* loader = getLoader(params.loaderId())) {
				return AssetDataHandler(loader->loadAsset(params));
			}
			return nullptr;
		}

		void setLoader(IAssetLoader2* loader) {
			const size_t lid = loader->loaderId();
			if (const IAssetLoader2* oldLoader = getLoader(lid)) {
				delete oldLoader;
			}
			_loaders[lid] = loader;
		}

	private:
		std::unordered_map<size_t, IAssetLoader2*> _loaders;
	};
	*/
}