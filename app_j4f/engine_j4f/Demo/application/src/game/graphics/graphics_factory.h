#pragma once

#include <Engine/Core/ref_ptr.h>
#include <Engine/Core/Math/mathematic.h>
#include <Engine/Utils/Json/Json.h>

#include <Engine/Graphics/Vulkan/vkRenderer.h>

#include "definitions.h"

#include <array>
#include <cstdint>
#include <memory>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>
#include <unordered_map>
#include <utility>

namespace engine {
    struct MeshGraphicsDataBuffer;
}

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
        uint16_t semanticMask = 0u;
        uint8_t graphicsBufferId = 0u;

        MeshDescription(uint16_t id, std::string&& path, uint8_t latency, uint16_t semanticMask, uint8_t graphicsBufferId) :
            id(id), path(std::move(path)), latency(latency), semanticMask(semanticMask), graphicsBufferId(graphicsBufferId) {}
    };

    struct DrawParams {
        vulkan::CullMode cullMode = vulkan::CullMode::CULL_MODE_BACK;
        vulkan::VulkanBlendMode blendMode = vulkan::CommonBlendModes::blend_none;
        bool depthTest = true;
        bool depthWrite = true;
        bool stencilTest = false;
        bool stencilWrite = false;
        uint8_t stencilRef = 0u;
        uint8_t stencilFunction = 0u;
    };

    struct GraphicsDescription {
        GraphicsType type = GraphicsType::None;
        size_t gpuProgramId = 0u;
        std::vector<std::tuple<size_t, float, uint8_t>> * animations = nullptr; // animationid + weight + id
        uint16_t descriptor = 0u;
        DrawParams drawParams;
        std::vector<uint16_t> textures;
        //...
    };

    struct ObjectDescription {
        std::string description;
        int16_t order = 0;
        engine::RotationsOrder rotationsOrder = engine::RotationsOrder::RO_XYZ;
        engine::vec3f scale = engine::vec3f(1.0f);
        engine::vec3f rotation = engine::vec3f(0.0f);
        engine::vec3f position = engine::vec3f(0.0f);
        std::vector<ObjectDescription> children;
    };

    class GraphicsFactory {
    public:
        GraphicsFactory();
        ~GraphicsFactory();

        void loadObjects(std::string_view path, std::function<void()>&& onComplete);
        void loadObjects(const engine::Json& json, std::function<void()>&& onComplete);

    //private:
        std::unordered_map<std::string, GraphicsDescription> _descriptions;
        std::unordered_map<std::string, ObjectDescription> _objects;

        std::vector<GPUProgramDescription> _gpuPrograms;
        std::vector<AnimationDescription> _animations;
        std::vector<MeshDescription> _meshes;
        std::vector<std::string> _textures;
        std::unordered_map<std::string, std::vector<std::tuple<size_t, float, uint8_t>>> _animationSets;

        std::vector<std::unique_ptr<engine::MeshGraphicsDataBuffer>> _meshGraphicsBuffers;
    };
}