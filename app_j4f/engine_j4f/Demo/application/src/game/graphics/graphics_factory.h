#pragma once

#include <Engine/Core/ref_ptr.h>
#include <Engine/Utils/Json/Json.h>

#include "definitions.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <unordered_map>
#include <utility>

namespace game {

    struct GPUProgramDescription {
        enum class Module : uint8_t {
            Vertex = 0u, Fragment, Geometry, Compute, Count
        };

        std::array<std::string, static_cast<size_t>(Module::Count)> modules;
    };

    struct AnimationDescription {
        std::string path;
        uint8_t id = 0u;
        float speed = 1.0f;
        bool infinity = true;
    };

    struct MeshDescription {
        std::string path;
        uint8_t latency = 1u;
    };

    struct GraphicsDescription {
        enum class Type : uint8_t {
            None = 0u, Reference = 1u, Mesh, Particles, Plane,
        };

        Type type = Type::None;
        uint16_t order = 0u;

        engine::ref_ptr<GPUProgramDescription> gpuProgram;
        std::vector<std::pair<engine::ref_ptr<AnimationDescription>, float>> animations; // animation + weight
        std::variant<std::monostate, MeshDescription> typeDescription;
        //...
    };

    struct ObjectDescription {
        engine::ref_ptr<GraphicsDescription> description;
        std::vector<ObjectDescription> children;
    };

    class GraphicsFactory {
    public:
        void loadObjects(std::string_view path);
        void loadObjects(const engine::Json& json);

    private:
        std::unordered_map<std::string, GraphicsDescription> _descriptions;
        std::unordered_map<std::string, ObjectDescription> _objects;
        std::unordered_map<std::string, GPUProgramDescription> _gpuPrograms;
        std::unordered_map<std::string, AnimationDescription> _animations;
    };
}