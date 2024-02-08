#pragma once

#include <Engine/Core/Common.h>

#include <cstdint>

namespace game {
    inline static constexpr uint16_t kInvalidEntityType = 0xffffu;
    class Entity {
    public:
        virtual ~Entity() = default;
        template <typename T>
        Entity(const T*) : _type(engine::UniqueTypeId<Entity>::getUniqueId<T>()) {}
        inline uint16_t type() const noexcept { return _type; }

    private:
        uint16_t _type = kInvalidEntityType;
    };
}