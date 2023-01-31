#pragma once

#include <cstddef> // for size_t
#include <vector>

#include <algorithm>

namespace engine {

	template <typename T>
	class MemoryChunk final {
	public:
        explicit MemoryChunk(const size_t sz) : _memSize(sz), _last(0), _holesCount(0), _memory(static_cast<T*>(malloc(sizeof(T) * _memSize))) {
            _holes.resize(_memSize);
            _holesMap.resize(_memSize);
            std::fill(_holes.begin(), _holes.end(), _memSize);
        }

        ~MemoryChunk() {
            for (size_t i = 0; i < _last; ++i) {
                if (_holesMap[i]) continue;
                T* object = at(i);
                object->~T(); // destructor call
            }

            if (_memory) {
                free(_memory);
                _memory = nullptr;
            }
            
            _last = 0;
            _holesCount = 0;
            _holes.clear();
            _holesMap.clear();
        }

        MemoryChunk(MemoryChunk&& chunk) noexcept : 
            _memSize(chunk._memSize),
            _last(chunk._last),
            _holesCount(chunk._holesCount),
            _memory(chunk._memory),
            _holes(std::move(chunk._holes)),
            _holesMap(std::move(chunk._holesMap))
        {
            chunk._memory = nullptr;
            chunk._last = 0;
        }

        MemoryChunk& operator= (MemoryChunk&& chunk) noexcept {
            _memSize = chunk._memSize;
            _last = chunk._last;
            _holesCount = chunk._holesCount;
            _memory = chunk._memory;
            _holes = std::move(chunk._holes);
            _holesMap = std::move(chunk._holesMap);

            chunk._memory = nullptr;
            chunk._last = 0;
        }

        MemoryChunk(const MemoryChunk&) = delete;
        MemoryChunk& operator= (const MemoryChunk&) = delete;

        template <typename... ARGS>
        inline T* createObject(ARGS&&... args) {
            if (_holesCount != 0) {
                const size_t position = _holes[--_holesCount];
                T* obj = new(at(position)) T(std::forward<ARGS>(args)...); // placement new
                _holesMap[position] = false;
                return obj;
            }

            if (_last >= _memSize) return nullptr;

            const size_t position = _last++;
            T* obj = new(at(position)) T(std::forward<ARGS>(args)...); // placement new

            return obj;
        }

        inline bool destroyObject(T* object) {
            if (object >= &_memory[0] && object < &_memory[_last]) {
                const auto i = static_cast<size_t>(object - &_memory[0]);

                if (_holesMap[i]) { return false; } // ���� ��� ����� - �� �� ������ ������

                object->~T(); // destructor call
                _holes[_holesCount++] = i; // holes
                _holesMap[i] = true;

                return true;
            }

            return false;
        }

        [[nodiscard]] inline size_t count() const noexcept { return _last - _holesCount;} // ���������� ��������, ��� �����

        [[nodiscard]] inline size_t size() const noexcept { return _last; } // ���������� ��������, ������� �����

        inline T* operator[] (const size_t i) const {
            return (i >= _last || _holesMap[i]) ? nullptr : at(i);
        }

	private:
        inline T* at(const size_t i) const {
            return &(_memory[i]);
        }

        size_t _memSize;
        size_t _last;
        size_t _holesCount;

        alignas(T) T* _memory;

        std::vector<size_t> _holes;
        std::vector<bool> _holesMap; // ��� ����������� ����� ��� ������ �� �������
	};
}