#pragma once

#include <memory>

namespace engine {

    template <typename T>
    class owned_ptr {
    public:
        using element_type = T;

        owned_ptr() = default;
        ~owned_ptr() {
            if (_ptr) {
                delete _ptr;
                _ptr = nullptr;
            }
        }

        explicit owned_ptr(T* ptr) : _ptr(ptr) {}
        owned_ptr(const owned_ptr&) = delete;
        owned_ptr(owned_ptr&& p) : _ptr(p._ptr) { p._ptr = nullptr; }

        inline owned_ptr& operator= (const owned_ptr& p) = delete;
        inline owned_ptr& operator= (owned_ptr&& p) noexcept {
            _ptr = p._ptr;
            p._ptr = nullptr;
            return *this;
        };

        inline bool operator== (const owned_ptr& p) const noexcept { return _ptr == p._ptr; }
        inline bool operator== (const T* p) const noexcept { return _ptr == p; }

        inline T* operator->() noexcept { return _ptr; }
        inline const T* operator->() const noexcept { return _ptr; }
        inline T* get() noexcept { return _ptr; }
        inline const T* get() const noexcept { return _ptr; }

        inline explicit operator bool() const noexcept { return _ptr != nullptr; }

        [[maybe_unused]] T* detach() noexcept {
            T* result = _ptr;
            _ptr = nullptr;
            return result;
        }

    private:
        T* _ptr = nullptr;
    };

    template <typename T, typename...Args>
    inline owned_ptr<T> make_owned(Args&&...args) {
        return owned_ptr<T>(new T(std::forward<Args>(args)...));
    }
}