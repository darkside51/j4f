#include "MemoryLeakChecker.h"

#ifdef _DEBUG

#ifndef j4f_PLATFORM_WINDOWS

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <mutex>

namespace {
    constexpr uint8_t kAlign = 16u;

    struct MemInfo {
        uint64_t allocationId = 0u;
        uint32_t line = 0u;
        const char *file = nullptr;
        size_t bytes = 0u;

        MemInfo *left = nullptr;
        MemInfo *right = nullptr;


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
    };

    MemInfo *currentInfo = nullptr;
    std::mutex info_mutex;

    MemInfo *record_info(uint64_t allocationId, size_t bytes, uint32_t line, const char *file) {
        std::lock_guard<std::mutex> lock(info_mutex);
        if (currentInfo == nullptr) {
            currentInfo = new MemInfo(allocationId, bytes, line, file);
        } else {
            auto info = new MemInfo(allocationId, bytes, line, file);
            info->left = currentInfo;
            currentInfo->right = info;
            currentInfo = currentInfo->right;
        }
        return currentInfo;
    }

    void erase_info(MemInfo *mi) {
        std::lock_guard<std::mutex> lock(info_mutex);
        if (mi->left) {
            mi->left->right = mi->right;
        }

        if (mi->right) {
            mi->right->left = mi->left;
        }

        if (mi == currentInfo) {
            currentInfo = currentInfo->left;
        }

        delete mi;
        mi = nullptr;
    }
}

thread_local const char *__file__ = "<unknown>";
thread_local uint32_t __line__ = 0u;

void operator delete(void *ptr) {
    if (ptr == nullptr) return;

    // un_poison memory
    const auto memory = reinterpret_cast<size_t *>(ptr) - kAlign / sizeof(size_t);
    const auto memoryAsNum = *(memory);
    erase_info(reinterpret_cast<MemInfo *>(memoryAsNum));
    free(memory);
}

void *operator new(size_t size) {
    static std::atomic<uint64_t> _allocationId = 0U;
    const uint64_t allocId = _allocationId.fetch_add(1u, std::memory_order_relaxed);

    void *result = malloc(size + kAlign);
    // poison memory
    *reinterpret_cast<size_t *>(result) = reinterpret_cast<size_t>(record_info(allocId, size, __line__, __file__));
    __line__ = 0u;
    __file__ = "<unknown>";
    return static_cast<void *>(reinterpret_cast<uint8_t *>(result) + kAlign);
}

//void *operator new(size_t size, const char* file, size_t line) {
//    static std::atomic<uint64_t> _allocationId = 0U;
//    const uint64_t allocId = _allocationId.fetch_add(1u, std::memory_order_relaxed);
//
//    void *result = malloc(size + kAlign);
//    // poison memory
//    *reinterpret_cast<size_t *>(result) = reinterpret_cast<size_t>(record_info(allocId, size, line, file));
//    return static_cast<void *>(reinterpret_cast<uint8_t *>(result) + kAlign);
//}

namespace engine {
    void MemoryChecker::checkLeaks() const {
        size_t leaks_count = 0u;
        uint64_t bytes = 0U;
        MemInfo *memory = currentInfo;
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

        currentInfo = nullptr;

        if (leaks_count) {
            printf("detected memory leaks: %lu / %llu bytes\n", leaks_count, bytes);
        }
    }
}

#endif // j4f_PLATFORM_WINDOWS

#endif _DEBUG