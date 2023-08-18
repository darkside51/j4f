#pragma once

#include "TextureHandler.h"
#include "../../Core/Cache.h"
#include "../../Core/Hash.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <utility>

namespace engine {

    struct CacheParams {
        bool storeForever = false;
    };

    class TextureCache final : public ICache {
    public:
        using key_type = std::string;
        using value_type = TexturePtr;

        ~TextureCache() override {
            // fix for destroy texture cache before textures
            _map.execute([](value_type & value){
                value->m_flags = TextureHandler::Flags::None;
            });
        }

        template <typename KEY = key_type>
        inline bool hasValue(KEY&& key) const noexcept { return _map.hasValue(key); }

        template <typename KEY = key_type, typename VAL = value_type>
        inline void setValue(KEY&& key, VAL&& value, CacheParams const & p) {
            auto && [name, pointer] = *_map.setValueExt(key, std::move(value));
            value_type & ptr = const_cast<value_type &>(pointer);
            ptr->m_key = name;
            ptr->m_flags = p.storeForever ? TextureHandler::Flags::ForeverInCache : TextureHandler::Flags::Cached;
        }

        template <typename KEY = key_type>
        inline const value_type& getValue(KEY&& key) noexcept { return _map.getValue(key); }

        template <typename KEY = key_type, typename F, typename ...Args>
        inline const value_type& getOrSetValue(KEY&& key, F&& f, CacheParams const & p, Args&&... args) {
            auto && [name, pointer] = *_map.getOrCreateExt(key, std::forward<F>(f), std::forward<Args>(args)...);
            value_type & ptr = const_cast<value_type &>(pointer);
            ptr->m_key = name;
            ptr->m_flags = p.storeForever ? TextureHandler::Flags::ForeverInCache : TextureHandler::Flags::Cached;
            return ptr;
        }

        inline void onTextureFree(value_type const & value) noexcept {
            onTextureFree(value.get());
        }

        inline void onTextureFree(value_type::element_type const * value) noexcept {
            if (value->m_flags == TextureHandler::Flags::Cached) {
                _map.erase(value->m_key);
            }
        }

        inline void eraseTexture(value_type::element_type const * value) noexcept { // force erase (for erase forever stored values available)
            _map.erase(value->m_key);
        }

    private:
        TsUnorderedMap<key_type, value_type, Hasher<key_type>, std::equal_to<>> _map;
    };

}