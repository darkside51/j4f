#pragma once

#include "MemoryChunk.h"

namespace engine {

    template <typename T>
    struct MemoryChunkChain final {
        MemoryChunk<T> chunk;
        MemoryChunkChain* next;

        MemoryChunkChain(const size_t sz) : chunk(sz), next(nullptr) {}

        ~MemoryChunkChain() {
            next = nullptr;
        }
    };

    template <typename T>
    class ChunkedMemoryPool final {
        using Chunk = MemoryChunkChain<T>;
    public:
        ChunkedMemoryPool(const size_t sz) : _chunksCount(1), _first(new Chunk(sz)), _used(_first) {}
        ~ChunkedMemoryPool() {
            _used = first;
            while (_used) {
                Chunk* tmp = _used->next;
                delete _used;
                _used = tmp;
            }
        }

        template <typename... ARGS>
        inline T* createObject(ARGS&&... args) {
            T* obj = _used->chunk.createObject(std::forward<Args>(args)...);
            while (!obj) {
                if (_used->next == nullptr) {
                    _used->next = new Chunk(sz);
                    ++_chunksCount;
                }

                _used = _used->next;
                obj = _used->chunk.createObject(std::forward<Args>(args)...);
            }
        }

        inline void destroyObject(T* object) {
            _used = _first;
            while (_used && !_used->chunk.destroyObject(object)) {
                _used = _used->next;
            }
        }

    private:
        uint16_t _chunksCount;
        Chunk* _first;
        Chunk* _used; // будем запоминать последний, в котором происходили какие - либо операции, чтоб начинать следующие действия с него
    };

}