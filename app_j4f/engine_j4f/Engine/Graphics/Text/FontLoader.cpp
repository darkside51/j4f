#include "FontLoader.h"
#include "../../Core/Engine.h"
#include "../../Core/Cache.h"
#include "../Graphics.h"
#include "../../Core/Threads/ThreadPool.h"
#include "../../Core/Threads/Synchronisations.h"
#include "../../Utils/Debug/Profiler.h"
#include <string>

namespace engine {

	void FontLoader::addCallback(Font* f, const FontLoadingCallback& c) {
		AtomicLock lock(_callbacksLock);
		_callbacks[f].emplace_back(c);
	}

	void FontLoader::executeCallbacks(Font* f, const AssetLoadingResult result) {
		std::vector<FontLoadingCallback> callbacks;
		{
			AtomicLock lock(_callbacksLock);
			auto it = _callbacks.find(f);
			if (it != _callbacks.end()) {
				callbacks = std::move(it->second);
				_callbacks.erase(it);
			}
		}

		for (auto&& c : callbacks) {
			c(f, result);
		}
	}

	void FontLoader::loadAsset(Font*& v, const FontLoadingParams& params, const FontLoadingCallback& callback) {
		PROFILE_TIME_SCOPED_M(fontLoading, params.file)
		auto&& engine = Engine::getInstance();
		auto&& cache = engine.getModule<CacheManager>()->getCache<std::string, Font*>();

		if (params.flags->use_cache) {
			if (v = cache->getValue(params.file)) {
				if (callback) {

					callback(v, AssetLoadingResult::LOADING_SUCCESS);

					/*switch (v->generationState()) {
					case vulkan::VulkanTextureCreationState::NO_CREATED:
						callback(v, AssetLoadingResult::LOADING_ERROR);
						break;
					case vulkan::VulkanTextureCreationState::CREATION_COMPLETE:
						callback(v, AssetLoadingResult::LOADING_SUCCESS);
						break;
					default:
						addCallback(v, callback);
						break;
					}*/
				}
				return;
			}
		}


		v = cache->getOrSetValue(params.file, [](const std::string& fileName) {
			auto&& fontsManager = Engine::getInstance().getModule<Graphics>()->getFontsManager();
			return new Font(fontsManager->getLibrary(), fileName);
		}, params.file);


		if (callback) {
			callback(v, AssetLoadingResult::LOADING_SUCCESS);
		}

	}

}