#pragma once

#include <Engine/Core/Math/mathematic.h>

#include <Engine/Core/Hierarchy.h>
#include <Engine/Core/ref_ptr.h>
#include <Engine/Graphics/Scene/NodeGraphicsLink.h>
#include <Engine/Utils/Debug/Assert.h>

#include <cstdint>
#include <string>
#include <string_view>

namespace game {
    using NodeHR = engine::HierarchyRaw<engine::Node>;
    using NodePtr = engine::ref_ptr<NodeHR>;

    class MapObject {
    public:
        MapObject() : _name("") {}
        MapObject(std::string && name) : _name(std::move(name)) {}
        MapObject(const std::string & name) : _name(name) {}
        MapObject(std::string_view name) : _name(name) {}

        template <typename VEC3>
        void setPosition(VEC3 position) {
            setTransformation(position, _position);
        }

        template <typename VEC3>
        void setRotation(VEC3 rotation) {
            setTransformation(rotation, _rotation);
        }

        template <typename VEC3>
        void setScale(VEC3 scale) {
            setTransformation(scale, _scale);
        }

        void setRotationsOrder(engine::RotationsOrder order) noexcept {
            if (_rotationsOrder != order) {
                _transformDirty = true;
                _rotationsOrder = order;
            }
        }

        const engine::vec3f & getPosition() const noexcept { return _position; }
        const engine::vec3f & getRotation() const noexcept { return _rotation; }
        const engine::vec3f & getScale() const noexcept { return _scale; }

        bool transformDirty() const noexcept { return _transformDirty; }
        const std::string & name() const noexcept { return _name; }

        void assignNode(NodePtr node) noexcept {
            if (!_node) {
                _node = node;
            } else {
                ENGINE_BREAK
            }
        }

        void updateTransform() noexcept;

    private:
        template <typename VEC3>
        void setTransformation(VEC3 value, engine::vec3f & arg) {
            constexpr float kEps = 0.1f;
            if (!engine::compare(value, arg, kEps)) {
                return;
            }

            if constexpr (std::is_reference_v<VEC3>) {
                arg = value;
            } else {
                arg = std::move(value);
            }

            _transformDirty = true;
        }

        engine::RotationsOrder _rotationsOrder = engine::RotationsOrder::RO_XYZ;
        engine::vec3f _position = {0.0f, 0.0f, 0.0f};
        engine::vec3f _rotation = {0.0f, 0.0f, 0.0f};
        engine::vec3f _scale = {1.0f, 1.0f, 1.0f};
        bool _transformDirty = false;

        std::string _name;
        NodePtr _node;
    };
}