#pragma once

#include <stdexcept>

namespace engine {

    struct ThrowOnCopy {
        ThrowOnCopy() = default;
        ThrowOnCopy(const ThrowOnCopy&) { throw std::runtime_error("copy no copyable object with wrapper!"); }
        ThrowOnCopy(ThrowOnCopy&&) = default;
        ThrowOnCopy& operator=(ThrowOnCopy&&) = default;
    };

    template<typename T>
    struct CopyWrapper : ThrowOnCopy {
        CopyWrapper(T&& t) : target(std::forward<T>(t)) { }

        CopyWrapper(CopyWrapper&&) = default;

        CopyWrapper(const CopyWrapper& other)
                : ThrowOnCopy(other),                             // this will throw
                  target(std::move(const_cast<T&>(other.target))) // never reached
        { }

        template<typename... Args>
        inline auto operator() (Args&&... args) { return target(std::forward<Args>(args)...); }

        T target;
    };

    template<typename T>
    inline CopyWrapper<T> copy_wrapper(T&& t) noexcept { return { std::forward<T>(t) }; }
}