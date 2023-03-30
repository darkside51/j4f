#pragma once

#include <cstdint>
#include <string>
#include <fmt/format.h>

namespace engine {
    struct Version_ids {
        uint8_t patch;
        uint8_t minor;
        uint8_t major;
    };

    union Version {
    public:
        Version(const uint8_t major, const uint8_t minor, const uint8_t patch) : _ids {patch, minor, major} { }
        inline uint32_t operator()() const noexcept { return _value; }
        inline const Version_ids* operator->() const noexcept { return &_ids; }
        inline std::string str() const {
            return fmt::format("{}.{}.{}", _ids.major, _ids.minor, _ids.patch);
        }
    private:
        Version_ids _ids;
        uint32_t _value;
    };
}