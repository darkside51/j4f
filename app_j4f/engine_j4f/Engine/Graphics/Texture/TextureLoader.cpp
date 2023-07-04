#include "TextureLoader.h"

#include "../../Core/Engine.h"
#include "../../Core/Cache.h"
#include "../../File/FileManager.h"
#include "../Graphics.h"
#include "../Vulkan/vkTexture.h"
#include "../../Utils/Debug/Profiler.h"

//
#include "TextureCache.h"

namespace engine {

	void TextureLoader::addCallback(vulkan::VulkanTexture* t, const TextureLoadingCallback& c) {
		AtomicLock lock(_callbacksLock);
		_callbacks[t].emplace_back(c);
	}

	void TextureLoader::executeCallbacks(vulkan::VulkanTexture* t, const AssetLoadingResult result) {
		std::vector<TextureLoadingCallback> callbacks;
		{
			AtomicLock lock(_callbacksLock);
			auto it = _callbacks.find(t);
			if (it != _callbacks.end()) {
				callbacks = std::move(it->second);
				_callbacks.erase(it);
			}
		}

		for (auto&& c : callbacks) {
			c(t, result);
		}
	}

	vulkan::VulkanTexture* TextureLoader::createTexture(const TextureLoadingParams& params, const TextureLoadingCallback& callback) {
		auto&& engine = Engine::getInstance();
		auto&& renderer = engine.getModule<engine::Graphics>()->getRenderer();

		vulkan::VulkanTexture* texture;
		if (params.texData) {
			texture = new vulkan::VulkanTexture(renderer, params.texData->width(), params.texData->height(), 1);
		} else {
			auto* fm = engine.getModule<engine::FileManager>();
			int width, height, channels;
			TextureData::getInfo(fm->getFullPath(params.files[0]).c_str(), &width, &height, &channels); // texture dimensions + channels without load

			texture = new vulkan::VulkanTexture(renderer, width, height, 1);
		}

		if (params.flags->async) {
			if (callback) { addCallback(texture, callback); }

			engine.getModule<AssetManager>()->getThreadPool()->enqueue(TaskType::COMMON, [params, texture](const CancellationToken& token) {
				PROFILE_TIME_SCOPED_M(textureLoading, params.files[0])
				if (params.texData) {
					if (!params.texData->operator bool()) {
						executeCallbacks(texture, AssetLoadingResult::LOADING_ERROR);
						texture->noGenerate();
						return;
					}
					texture->create(params.texData->data(), params.texData->format(), params.texData->bpp(), params.textureFlags->useMipMaps, true, params.imageViewTypeForce);
				} else {
					const size_t size = params.files.size();
					std::vector<TextureData> imgs;
					imgs.reserve(size);
					std::vector<const void*> imgsData(size);

					for (size_t i = 0; i < size; ++i) {
						imgs.emplace_back(params.files[i], params.formatType);
						
						if (!imgs[i]) {
							executeCallbacks(texture, AssetLoadingResult::LOADING_ERROR);
							texture->noGenerate();
							return;
						}

						imgsData[i] = imgs[i].data();
					}

					texture->create(imgsData.data(), imgs.size(), imgs[0].format(), imgs[0].bpp(), params.textureFlags->useMipMaps, true, params.imageViewTypeForce);
				}

				if (params.imageLayout != VK_IMAGE_LAYOUT_MAX_ENUM) {
					texture->createSingleDescriptor(params.imageLayout, params.binding);
				}

				executeCallbacks(texture, AssetLoadingResult::LOADING_SUCCESS);
			});
		} else {
			PROFILE_TIME_SCOPED_M(textureLoading, params.files[0])
			if (params.texData) {
				if (!params.texData->operator bool()) {
					executeCallbacks(texture, AssetLoadingResult::LOADING_ERROR);
					texture->noGenerate();
					return texture;
				}
				texture->create(params.texData->data(), params.texData->format(), params.texData->bpp(), params.textureFlags->useMipMaps, true, params.imageViewTypeForce);
			} else {
				const size_t size = params.files.size();
				std::vector<TextureData> imgs;
				imgs.reserve(size);
				std::vector<const void*> imgsData(size);

				for (size_t i = 0; i < size; ++i) {
					imgs.emplace_back(params.files[i], params.formatType);

					if (!imgs[i]) {
						if (callback) { callback(texture, AssetLoadingResult::LOADING_ERROR); }
						texture->noGenerate();
						return texture;
					}

					imgsData[i] = imgs[i].data();
				}

				texture->create(imgsData.data(), imgs.size(), imgs[0].format(), imgs[0].bpp(), params.textureFlags->useMipMaps, params.textureFlags->deffered, params.imageViewTypeForce);
			}

			if (params.imageLayout != VK_IMAGE_LAYOUT_MAX_ENUM) {
				texture->createSingleDescriptor(params.imageLayout, params.binding);
			}

			if (callback) { callback(texture, AssetLoadingResult::LOADING_SUCCESS); }
		}

//        auto&& cache = engine.getModule<CacheManager>()->getCache<TextureCache>();
//        TexturePtr tptr = make_linked<TextureHandler>(std::move(*texture));
//        cache->setValue("test_texture", std::move(tptr), {true});

		return texture;
	}

	void TextureLoader::loadAsset(vulkan::VulkanTexture*& v, const TextureLoadingParams& params, const TextureLoadingCallback& callback) {

		auto&& engine = Engine::getInstance();
		auto&& cache = engine.getModule<CacheManager>()->getCache<std::string, vulkan::VulkanTexture*>();

		if (params.flags->use_cache) {
			if (params.cacheName.empty()) {
				if (params.files.size() > 1) {
					size_t length = 0;
					for (auto&& f : params.files) {
						length += f.length();
					}

					std::string cacheKey;
					cacheKey.reserve(length);

					for (auto&& f : params.files) {
						cacheKey += f;
					}

                    v = cache->getValue(cacheKey);
                    if (v) {
						if (callback) {
							switch (v->generationState()) {
							case vulkan::VulkanTextureCreationState::NO_CREATED:
								callback(v, AssetLoadingResult::LOADING_ERROR);
								break;
							case vulkan::VulkanTextureCreationState::CREATION_COMPLETE:
								callback(v, AssetLoadingResult::LOADING_SUCCESS);
								break;
							default:
								addCallback(v, callback);
								break;
							}
						}
						return;
					}

					v = cache->getOrSetValue(cacheKey, [](const TextureLoadingParams& params, const TextureLoadingCallback& callback) {
						return createTexture(params, callback);
						}, params, callback);
				} else {
                    v = cache->getValue(params.files[0]);
                    if (v) {
						if (callback) {
							switch (v->generationState()) {
							case vulkan::VulkanTextureCreationState::NO_CREATED:
								callback(v, AssetLoadingResult::LOADING_ERROR);
								break;
							case vulkan::VulkanTextureCreationState::CREATION_COMPLETE:
								callback(v, AssetLoadingResult::LOADING_SUCCESS);
								break;
							default:
								addCallback(v, callback);
								break;
							}
						}
						return;
					}

					v = cache->getOrSetValue(params.files[0], [](const TextureLoadingParams& params, const TextureLoadingCallback& callback) {
						return createTexture(params, callback);
						}, params, callback);
				}
			} else {
                v = cache->getValue(params.cacheName);
                if (v) {
					if (callback) {
						switch (v->generationState()) {
						case vulkan::VulkanTextureCreationState::NO_CREATED:
							callback(v, AssetLoadingResult::LOADING_ERROR);
							break;
						case vulkan::VulkanTextureCreationState::CREATION_COMPLETE:
							callback(v, AssetLoadingResult::LOADING_SUCCESS);
							break;
						default:
							addCallback(v, callback);
							break;
						}
					}
					return;
				}

				v = cache->getOrSetValue(params.cacheName, [](const TextureLoadingParams& params, const TextureLoadingCallback& callback) {
					return createTexture(params, callback);
					}, params, callback);
			}
		} else {
			v = createTexture(params, callback);
		}
	}

}