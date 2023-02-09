#pragma once

#include <type_traits>
#include <random>
#include <memory>
#include <cstdint>
#include <ctime>

namespace engine {

    template <typename T> struct randomTypeAcessor  { using value = void; };
    template<> struct randomTypeAcessor<int8_t>     { using value = int; };
    template<> struct randomTypeAcessor<int16_t>    { using value = int; };
    template<> struct randomTypeAcessor<int32_t>    { using value = int; };
    template<> struct randomTypeAcessor<int64_t>    { using value = int; };
    template<> struct randomTypeAcessor<uint8_t>    { using value = int; };
    template<> struct randomTypeAcessor<uint16_t>   { using value = int; };
    template<> struct randomTypeAcessor<uint32_t>   { using value = int; };
    template<> struct randomTypeAcessor<uint64_t>   { using value = int; };

    template<> struct randomTypeAcessor<float>      { using value = float; };
    template<> struct randomTypeAcessor<double>     { using value = float; };

    static thread_local std::unique_ptr<std::mt19937> generator;

    template <typename T>
    inline T random(const T min, const T max) {
        if (!generator) {
            generator = std::make_unique<std::mt19937>(std::clock());
        }

        if constexpr (std::is_same_v<typename randomTypeAcessor<T>::value, int>) {
            std::uniform_int_distribution<T> distribution(min, max);
            return distribution(*generator);
        }

        if constexpr (std::is_same_v<typename randomTypeAcessor<T>::value, float>) {
            std::uniform_real_distribution<T> distribution(min, max);
            return distribution(*generator);
        }

        return T(0);
    }

}