#pragma once

#include <cstdio>

namespace engine {

    struct TerminateOnCopy {
        TerminateOnCopy() = default;
        TerminateOnCopy(const TerminateOnCopy&) {
            printf("copy no copyable object with wrapper!\n");
            std::terminate();
        }
        TerminateOnCopy(TerminateOnCopy&&) noexcept = default;
        TerminateOnCopy& operator=(TerminateOnCopy&&) noexcept = default;
    };

    template<typename T>
    struct CopyWrapper : TerminateOnCopy {
        CopyWrapper(T&& t) : target(std::forward<T>(t)) { }

        CopyWrapper(CopyWrapper&&) = default;

        CopyWrapper(const CopyWrapper& other)
                : TerminateOnCopy(other),                             // this will throw
                  target(std::move(const_cast<T&>(other.target))) // never reached
        { }

        template<typename... Args>
        inline auto operator() (Args&&... args) { return target(std::forward<Args>(args)...); }

        T target;
    };

    template<typename T>
    inline CopyWrapper<T> copy_wrapper(T&& t) noexcept { return { std::forward<T>(t) }; }
}