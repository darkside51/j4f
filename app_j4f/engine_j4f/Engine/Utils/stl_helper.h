#pragma once

#include <type_traits>

namespace engine {
    template< class Enum >
    inline constexpr std::underlying_type_t<Enum> to_underlying( Enum e ) noexcept {
        return static_cast<std::underlying_type_t<Enum>>(e);
    }

    template<typename Enum>
    inline bool checkEnumElements(Enum e, Enum element0) noexcept {
        return to_underlying(e) & to_underlying(element0);
    }

    template<typename Enum, typename... Enums>
    inline bool checkEnumElements(Enum e, Enum element0, Enums... elements) noexcept {
        if (checkEnumElements(e, element0)) {
            return checkEnumElements(e, elements...);
        }
        return false;
    }
}