#pragma once

#include <Engine/Core/ref_ptr.h>
#include <Engine/Utils/Json/Json.h>

#include "definitions.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>
#include <unordered_map>
#include <utility>

namespace game {

    struct GPUProgramDescription {
        enum class Module : uint8_t {
            Vertex = 0u, Fragment, Geometry, Compute, Count
        };
        std::string name;
        std::array<std::string, static_cast<size_t>(Module::Count)> modules;
    };

    struct AnimationDescription {
        std::string name;
        std::string path;
        float speed = 1.0f;
        bool infinity = true;
    };

    enum class GraphicsType : uint8_t {
        None = 0u, Reference = 1u, Mesh, Particles, Plane,
    };

    struct MeshDescription {
        uint16_t id = 0u;
        std::string path;
        uint8_t latency = 1u;

        MeshDescription(uint16_t id, std::string&& path, uint8_t latency) :
            id(id), path(std::move(path)), latency(latency) {}
    };

    struct GraphicsDescription {
        GraphicsType type = GraphicsType::None;
        size_t gpuProgramId = 0u;
        std::vector<std::tuple<size_t, float, uint8_t>> animations; // animationid + weight + id
        uint16_t descriptor = 0u;
        //...
    };

    struct ObjectDescription {
        engine::ref_ptr<GraphicsDescription> description;
        uint16_t order = 0u;
        std::vector<ObjectDescription> children;
    };

    class GraphicsFactory {
    public:
        void loadObjects(std::string_view path);
        void loadObjects(const engine::Json& json);

    private:
        std::unordered_map<std::string, GraphicsDescription> _descriptions;
        std::unordered_map<std::string, ObjectDescription> _objects;

        std::vector<GPUProgramDescription> _gpuPrograms;
        std::vector<AnimationDescription> _animations;
        std::vector<MeshDescription> _meshes;
    };
}