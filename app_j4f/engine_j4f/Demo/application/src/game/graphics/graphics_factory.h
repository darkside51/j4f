#pragma once

#include <Engine/Core/ref_ptr.h>
#include <Engine/Utils/Json/Json.h>

#include "definitions.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace game {

    struct GraphicsDescription{
        enum class Type : uint8_t {
            None = 0u, Mesh = 1u, Particles, Plane,
        };
        Type type = Type::None;
        uint16_t order = 0u;
        //...
    };

    struct ObjectDescription {
        engine::ref_ptr<GraphicsDescription> description;
        std::vector<ObjectDescription> children;
    };

    class GraphicsFactory {
    public:
        void loadObjects(std::string_view path);
        void loadObjects(engine::Json&& json);

    private:
        std::unordered_map<std::string, GraphicsDescription> _descriptions;
        std::unordered_map<std::string, ObjectDescription> _objects;
    };
}