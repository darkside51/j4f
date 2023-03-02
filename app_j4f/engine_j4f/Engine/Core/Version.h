#pragma once

#include <cstdint>

namespace engine {
    struct Version_ids {
        uint8_t patch;
        uint8_t minor;
        uint8_t major;
    };

    union Version {
    public:
        Version(const uint8_t major, const uint8_t minor, const uint8_t patch) : _ids {patch, minor, major} { }
        inline uint32_t operator()() const { return _value; }
        inline const Version_ids* operator->() const { return &_ids; }
    private:
        Version_ids _ids;
        uint32_t _value;
    };
}