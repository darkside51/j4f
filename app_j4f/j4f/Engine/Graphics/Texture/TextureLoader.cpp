#include "TextureLoader.h"

#include "../../Core/Engine.h"
#include "../../Core/Cache.h"
#include "../../Core/AssetManager.h"
#include "../../Core/Threads/ThreadPool.h"
#include "../../Core/Threads/Synchronisations.h"
#include "../../File/FileManager.h"
#include "../Graphics.h"
#include "../Vulkan/vkTexture.h"
#include "../../Utils/Debug/Profiler.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace engine {

	inline void getTextureInfo(const char* file, int* w, int* h, int* c) {
		stbi_info(file, w, h, c); // texture dimensions + channels without load
	}

	inline unsigned char* loadImageDataFromBuffer (const unsigned char* buffer, const size_t size, int* w, int* h, int* c){
		return stbi_load_from_memory(buffer, size, w, h, c, 4);
	}

	inline void freeImageData(unsigned char* img) {
		stbi_image_free(img);
	}

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
		FileManager* fm = engine.getModule<engine::FileManager>();
		int width, height, channels;
		getTextureInfo(fm->getFullPath(params.file).c_str(), &width, &height, &channels); // texture dimensions + channels without load

		auto&& renderer = engine.getModule<engine::Graphics>()->getRenderer();
		vulkan::VulkanTexture* texture = new vulkan::VulkanTexture(renderer, width, height, 1);

		if (params.flags->async) {
			if (callback) { addCallback(texture, callback); }

			engine.getModule<AssetManager>()->getThreadPool()->enqueue(TaskType::COMMON, 0, [params, texture](const CancellationToken& token) {
				PROFILE_TIME_SCOPED_M(textureLoading, params.file)
				FileManager* fm = engine::Engine::getInstance().getModule<engine::FileManager>();
				size_t fsize;
				char* imgBuffer = fm->readFile(params.file, fsize);

				int width, height, channels;
				unsigned char* img = loadImageDataFromBuffer(reinterpret_cast<unsigned char*>(imgBuffer), fsize, &width, &height, &channels);

				delete imgBuffer;

				if (img == nullptr) {
					executeCallbacks(texture, AssetLoadingResult::LOADING_ERROR);
					texture->noGenerate();
					return;
				}

				texture->create(img, VK_FORMAT_R8G8B8A8_UNORM, 32, params.textureFlags->useMipMaps, true);

				freeImageData(img);

				if (params.imageLayout != VK_IMAGE_LAYOUT_MAX_ENUM) {
					texture->createSingleDescriptor(params.imageLayout, params.binding);
				}

				executeCallbacks(texture, AssetLoadingResult::LOADING_SUCCESS);
			});
		} else {
			PROFILE_TIME_SCOPED_M(textureLoading, params.file)
			FileManager* fm = engine.getModule<engine::FileManager>();
			size_t fsize;
			const char* imgBuffer = fm->readFile(params.file, fsize);
			unsigned char* img = loadImageDataFromBuffer(reinterpret_cast<const unsigned char*>(imgBuffer), fsize, &width, &height, &channels);

			delete imgBuffer;

			if (img == nullptr) {
				if (callback) {
					callback(texture, AssetLoadingResult::LOADING_ERROR);
				}
				texture->noGenerate();
				return texture;
			}

			texture->create(img, VK_FORMAT_R8G8B8A8_UNORM, 32, params.textureFlags->useMipMaps, params.textureFlags->deffered);

			freeImageData(img);

			if (params.imageLayout != VK_IMAGE_LAYOUT_MAX_ENUM) {
				texture->createSingleDescriptor(params.imageLayout, params.binding);
			}

			if (callback) { callback(texture, AssetLoadingResult::LOADING_SUCCESS); }
		}

		return texture;
	}

	void TextureLoader::loadAsset(vulkan::VulkanTexture*& v, const TextureLoadingParams& params, const TextureLoadingCallback& callback) {

		auto&& engine = Engine::getInstance();
		auto&& cache = engine.getModule<CacheManager>()->getCache<std::string, vulkan::VulkanTexture*>();

		if (params.flags->use_cache) {
			if (v = cache->getValue(params.file)) {
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
		}

		if (params.flags->use_cache) {
			v = cache->getOrSetValue(params.file, [](const TextureLoadingParams& params, const TextureLoadingCallback& callback) {
				return createTexture(params, callback);
			}, params, callback);
		} else {
			v = createTexture(params, callback);
		}
	}

}