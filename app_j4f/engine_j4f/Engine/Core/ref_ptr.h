#pragma once

#include <stdexcept>
#include <memory>

namespace engine {

    template <typename T>
    class ref_ptr {
    public:
        using element_type = T;

        ref_ptr() = default;
        ref_ptr(T* ptr) : _ptr(ptr) {}
        ref_ptr(nullptr_t) : _ptr(nullptr) {}
        ref_ptr(std::unique_ptr<T> & ptr) : _ptr(ptr.get()) {}

        ~ref_ptr() { _ptr = nullptr; }

        ref_ptr(const ref_ptr & other) : _ptr(other._ptr) {}
        ref_ptr(ref_ptr && other) noexcept : _ptr(other._ptr) { other._ptr = nullptr; }

        ref_ptr& operator= (const ref_ptr& other) {
            if (&other == this) { return *this; }
            _ptr = other._ptr;
            return *this;
        }

        ref_ptr& operator= (ref_ptr&& other) noexcept {
            if (&other == this) { return *this; }
            _ptr = other._ptr;
            other._ptr = nullptr;
            return *this;
        }

        ref_ptr& operator= (nullptr_t) noexcept {
            _ptr = nullptr;
            return *this;
        }

        ref_ptr& operator= (element_type* ptr) noexcept {
            _ptr = ptr;
            return *this;
        }

        ref_ptr& operator= (std::unique_ptr<element_type> & ptr) noexcept {
            _ptr = ptr.get();
            return *this;
        }

        explicit inline operator bool() const noexcept { return _ptr != nullptr; }

        inline bool operator == (nullptr_t) noexcept { return _ptr == nullptr; }
        inline bool operator == (const ref_ptr& ptr) noexcept { return _ptr == ptr._ptr; }

        element_type& operator* () {
            if (_ptr) {
                return *_ptr;
            }
            throw std::runtime_error("incorrect operator*()");
        }

        const element_type& operator* () const {
            if (_ptr) {
                return *_ptr;
            }
            throw std::runtime_error("incorrect operator*() const");
        }

        inline element_type* operator->() noexcept { return _ptr; }
        inline const element_type* operator->() const noexcept { return _ptr; }

        inline element_type* get() noexcept { return _ptr; }
        inline const element_type* get() const noexcept { return _ptr; }

        inline void reset() noexcept { _ptr = nullptr; }

    private:
        T* _ptr = nullptr;
    };

    template <>
    class ref_ptr<void> {
    public:
        using element_type = void;

        ref_ptr() = default;
        ref_ptr(void* ptr) : _ptr(ptr) {}
        ref_ptr(nullptr_t) : _ptr(nullptr) {}
        ref_ptr(std::unique_ptr<void> & ptr) : _ptr(ptr.get()) {}

        ~ref_ptr() { _ptr = nullptr; }

        ref_ptr(const ref_ptr & other) : _ptr(other._ptr) {}
        ref_ptr(ref_ptr && other) noexcept : _ptr(other._ptr) { other._ptr = nullptr; }

        ref_ptr& operator= (const ref_ptr& other) {
            if (&other == this) { return *this; }
            _ptr = other._ptr;
            return *this;
        }

        ref_ptr& operator= (ref_ptr&& other) noexcept {
            if (&other == this) { return *this; }
            _ptr = other._ptr;
            other._ptr = nullptr;
            return *this;
        }

        ref_ptr& operator= (nullptr_t) noexcept {
            _ptr = nullptr;
            return *this;
        }

        ref_ptr& operator= (element_type* ptr) noexcept {
            _ptr = ptr;
            return *this;
        }

        ref_ptr& operator= (std::unique_ptr<void> & ptr) noexcept {
            _ptr = ptr.get();
            return *this;
        }

        explicit inline operator bool() const noexcept { return _ptr != nullptr; }

        inline bool operator == (nullptr_t) noexcept { return _ptr == nullptr; }

        inline element_type* operator->() noexcept { return _ptr; }
        inline const element_type* operator->() const noexcept { return _ptr; }

        inline element_type* get() noexcept { return _ptr; }
        inline const element_type* get() const noexcept { return _ptr; }
        inline void reset() noexcept { _ptr = nullptr; }
    private:
        void* _ptr = nullptr;
    };

    template<typename T>
    ref_ptr<T> make_ref(T & value) noexcept {
        return ref_ptr<T>(&value);
    }

    template<typename T>
    ref_ptr<T> make_ref(T * value) noexcept {
        return ref_ptr<T>(value);
    }

    template<typename T>
    ref_ptr<T> make_ref(std::unique_ptr<T> & value) noexcept {
        return ref_ptr<T>(value.get());
    }
}