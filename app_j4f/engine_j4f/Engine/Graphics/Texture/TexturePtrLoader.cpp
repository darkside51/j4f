#include "TexturePtrLoader.h"
#include "TextureHandler.h"

#include "../Graphics.h"
#include "../../File/FileManager.h"
#include "../../Utils/Debug/Profiler.h"

namespace engine {

    void TexturePtrLoader::addCallback(TexturePtr t, const TexturePtrLoadingCallback &c) {
        AtomicLock lock(_callbacksLock);
        _callbacks[t].emplace_back(c);
    }

    void TexturePtrLoader::executeCallbacks(TexturePtr t, const AssetLoadingResult result) {
        std::vector<TexturePtrLoadingCallback> callbacks;
        {
            AtomicLock lock(_callbacksLock);
            auto it = _callbacks.find(t);
            if (it != _callbacks.end()) {
                callbacks = std::move(it->second);
                _callbacks.erase(it);
            }
        }

        for (auto &&c: callbacks) {
            c(t, result);
        }
    }

    TexturePtr
    TexturePtrLoader::createTexture(const TexturePtrLoadingParams &params, const TexturePtrLoadingCallback &callback) {
        auto &&engine = Engine::getInstance();
        auto &&renderer = engine.getModule<engine::Graphics>().getRenderer();

        TexturePtr texture;
        if (params.texData) {
            texture = make_linked<TextureHandler>(renderer, params.texData->width(), params.texData->height(), 1);
        } else {
            auto &fm = engine.getModule<engine::FileManager>();
            int width, height, channels;
            TextureData::getInfo(fm.getFullPath(params.files[0]).c_str(), &width, &height,
                                 &channels); // texture dimensions + channels without load

            texture = make_linked<TextureHandler>(renderer, width, height, 1);
        }

        if (params.flags->async) {
            if (callback) { addCallback(texture, callback); }

            engine.getModule<AssetManager>().getThreadPool()->enqueue(TaskType::COMMON, [params, texture](
                    const CancellationToken &token) mutable {
                PROFILE_TIME_SCOPED_M(textureLoading, params.files[0])

                auto texture_value = texture->get();

                if (params.texData) {
                    if (!params.texData->operator bool()) {
                        executeCallbacks(texture, AssetLoadingResult::LOADING_ERROR);
                        texture_value->noGenerate();
                        return;
                    }
                    texture_value->create(params.texData->data(), params.texData->format(), params.texData->bpp(),
                                          params.textureFlags->useMipMaps, true, params.imageViewTypeForce);
                } else {
                    const size_t size = params.files.size();
                    std::vector<TextureData> imgs;
                    imgs.reserve(size);
                    std::vector<const void *> imgsData(size);

                    for (size_t i = 0; i < size; ++i) {
                        imgs.emplace_back(params.files[i], params.formatType);

                        if (!imgs[i]) {
                            executeCallbacks(texture, AssetLoadingResult::LOADING_ERROR);
                            texture_value->noGenerate();
                            return;
                        }

                        imgsData[i] = imgs[i].data();
                    }

                    texture_value->create(imgsData.data(), imgs.size(), imgs[0].format(), imgs[0].bpp(),
                                          params.textureFlags->useMipMaps, true, params.imageViewTypeForce);
                }

                if (params.imageLayout != VK_IMAGE_LAYOUT_MAX_ENUM) {
                    texture_value->createSingleDescriptor(params.imageLayout, params.binding);
                }

                executeCallbacks(texture, AssetLoadingResult::LOADING_SUCCESS);
            });
        } else {
            PROFILE_TIME_SCOPED_M(textureLoading, params.files[0])

            auto texture_value = texture->get();
            if (params.texData) {
                if (!params.texData->operator bool()) {
                    executeCallbacks(texture, AssetLoadingResult::LOADING_ERROR);
                    texture_value->noGenerate();
                    return texture;
                }
                texture_value->create(params.texData->data(), params.texData->format(), params.texData->bpp(),
                                      params.textureFlags->useMipMaps, params.textureFlags->deffered,
                                      params.imageViewTypeForce);
            } else {
                const size_t size = params.files.size();
                std::vector<TextureData> imgs;
                imgs.reserve(size);
                std::vector<const void *> imgsData(size);

                for (size_t i = 0; i < size; ++i) {
                    imgs.emplace_back(params.files[i], params.formatType);

                    if (!imgs[i]) {
                        if (callback) { callback(texture, AssetLoadingResult::LOADING_ERROR); }
                        texture_value->noGenerate();
                        return texture;
                    }

                    imgsData[i] = imgs[i].data();
                }

                texture_value->create(imgsData.data(), imgs.size(), imgs[0].format(), imgs[0].bpp(),
                                      params.textureFlags->useMipMaps, params.textureFlags->deffered,
                                      params.imageViewTypeForce);
            }

            if (params.imageLayout != VK_IMAGE_LAYOUT_MAX_ENUM) {
                texture_value->createSingleDescriptor(params.imageLayout, params.binding);
            }

            if (callback) { callback(texture, AssetLoadingResult::LOADING_SUCCESS); }
        }

        return texture;
    }

    ///////
    void TexturePtrLoader::loadAsset(asset_type &v, const TexturePtrLoadingParams &params,
                                     const TexturePtrLoadingCallback &callback) {
        auto &&engine = Engine::getInstance();
        auto &&cache = engine.getModule<CacheManager>().getCache<TextureCache>();

        if (params.flags->use_cache) {
            auto generate = [&v, &cache, &callback, &params](const std::string& name){
                v = cache->getValue(name);
                if (v) {
                    if (callback) {
                        switch (v->get()->generationState()) {
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

                v = cache->getOrSetValue(name, [](const TexturePtrLoadingParams& params, const TexturePtrLoadingCallback& callback) {
                    return createTexture(params, callback);
                }, params.cacheParams, params, callback);
            };

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

                    generate(cacheKey);
                } else {
                    generate(params.files[0]);
                }
            } else {
                generate(params.cacheName);
            }
        } else {
            v = createTexture(params, callback);
        }
    }
}