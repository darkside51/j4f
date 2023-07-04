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

        template <typename KEY = key_type>
        inline bool hasValue(KEY&& key) const noexcept { return _map.hasValue(key); }

        template <typename KEY = key_type, typename VAL = value_type>
        inline void setValue(KEY&& key, VAL&& value, CacheParams&& p) {
            auto && const_ptr = _map.setValue(key, std::move(value));
            value_type & ptr = const_cast<value_type &>(const_ptr);
            ptr->m_key = key;
            ptr->m_flags = p.storeForever ? TextureHandler::Flags::ForeverInCache : TextureHandler::Flags::Cached;
        }

        template <typename KEY = key_type>
        inline const value_type& getValue(KEY&& key) const noexcept { return _map.getValue(key).first; }

        inline void onTextureFree(value_type const & value) noexcept {
            onTextureFree(value.get());
        }

        inline void onTextureFree(TextureHandler const * value) noexcept {
            if (value->m_flags == TextureHandler::Flags::Cached) {
                _map.erase(value->m_key);
            }
        }

    private:
        TsUnorderedMap<key_type, value_type, Hasher<key_type>, std::equal_to<>> _map;
    };

}