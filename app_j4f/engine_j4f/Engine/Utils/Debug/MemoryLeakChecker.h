#pragma once

#ifdef _DEBUG

#ifdef j4f_PLATFORM_WINDOWS

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

// todo: how to fix placement new?
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
//#define new DEBUG_NEW // uncomment this to enable leaks normal paths output

// replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type

namespace engine {
    inline void enableMemoryLeaksDebugger() { _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); }
}
#else // j4f_PLATFORM_WINDOWS

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <mutex>

struct MemInfo {
    uint64_t allocationId = 0u;
    uint32_t line = 0u;
    const char *file = nullptr;
    size_t bytes = 0u;

    MemInfo *left = nullptr;
    MemInfo *right = nullptr;

    inline static MemInfo *last = nullptr;
    inline static std::mutex mutex;

    MemInfo(uint64_t allocationId, size_t bytes, uint32_t line, const char *file) : allocationId(allocationId),
                                                                                    bytes(bytes), line(line),
                                                                                    file(file) {}

    ~MemInfo() {
        left = nullptr;
        right = nullptr;
        allocationId = 0u;
        bytes = 0u;
        line = 0u;
        file = nullptr;
    }

    void *operator new(size_t size) {
        return malloc(size);
    }

    void operator delete(void *ptr) {
        free(ptr);
    }

    static MemInfo *next(uint64_t allocationId, size_t bytes, uint32_t line, const char *file) {
        std::lock_guard<std::mutex> lock(mutex);
        if (last == nullptr) {
            last = new MemInfo(allocationId, bytes, line, file);
        } else {
            auto info = new MemInfo(allocationId, bytes, line, file);
            info->left = last;
            last->right = info;
            last = last->right;
        }
        return last;
    }

    static void remove(MemInfo *mi) {
        std::lock_guard<std::mutex> lock(mutex);
        if (mi->left) {
            mi->left->right = mi->right;
        }

        if (mi->right) {
            mi->right->left = mi->left;
        }

        if (mi == last) {
            last = last->left;
        }

        delete mi;
        mi = nullptr;
    }
};

inline thread_local const char *__file__ = "<unknown>";
inline thread_local uint32_t __line__ = 0u;

constexpr uint8_t kAlign = 16u;

inline void operator delete(void *ptr) {
    if (ptr == nullptr) return;

    // unpoison memory
    const auto memory = reinterpret_cast<size_t *>(ptr) - kAlign / sizeof(size_t);
    const auto memoryAsNum = *(memory);
    MemInfo::remove(reinterpret_cast<MemInfo *>(memoryAsNum));
    free(memory);
}

inline void *operator new(size_t size) {
    static std::atomic<uint64_t> _allocationId = 0U;
    const uint64_t allocId = _allocationId.fetch_add(1u, std::memory_order_relaxed);

    void *result = malloc(size + kAlign);
    // poison memory
    *reinterpret_cast<size_t *>(result) = reinterpret_cast<size_t>(MemInfo::next(allocId, size, __line__, __file__));
    __line__ = 0u;
    __file__ = "<unknown>";
    return static_cast<void *>(reinterpret_cast<uint8_t *>(result) + kAlign);
}

//inline void *operator new(size_t size, const char* file, size_t line) {
//    static std::atomic<uint64_t> _allocationId = 0U;
//    const uint64_t allocId = _allocationId.fetch_add(1u, std::memory_order_relaxed);
//
//    void *result = malloc(size + kAlign);
//    *reinterpret_cast<size_t *>(result) = reinterpret_cast<size_t>(MemInfo::next(allocId, size, line, file));
//    return static_cast<void *>(reinterpret_cast<uint8_t *>(result) + kAlign);
//}
//#define DEBUG_NEW new(__FILE__, __LINE__)

//for g++, "new" is expanded only once. The key is the "&& 0" which makes it false and causes the real new to be used. For example,
//#define new (__file__=__FILE__, __line__=__LINE__) && 0 ? NULL : new

namespace engine {
    inline void enableMemoryLeaksDebugger() noexcept {}

    inline class MemoryChecker {
    public:
        ~MemoryChecker() {
            checkLeaks();
        }
    private:
        void checkLeaks() const {
            size_t leaks_count = 0u;
            uint64_t bytes = 0U;
            MemInfo *memory = MemInfo::last;
            while (memory) {
                ++leaks_count;
                bytes += memory->bytes;

                const std::string_view sv(memory->file);
                auto const str = sv.substr(sv.find_last_of('/') + 1u, std::string_view::npos);
                printf("memory leak detected (id: %llu / %llu bytes): file: %s, line: %u\n", memory->allocationId,
                       memory->bytes, str.data(),
                       memory->line);

                if (memory = memory->left) {
                    delete memory->right;
                    memory->right = nullptr;
                } else {
                    delete memory;
                    memory = nullptr;
                }
            }

            MemInfo::last = nullptr;

            if (leaks_count) {
                printf("detected memory leaks: %llu / %llu bytes\n", leaks_count, bytes);
            }
        }
    } memoryChecker; // must be a first allocation
}
#endif // j4f_PLATFORM_WINDOWS


#endif // _DEBUG
