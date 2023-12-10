#pragma once

#include <Engine/Core/Hierarchy.h>
#include <Engine/Core/Ref_ptr.h>
#include <Engine/Graphics/Scene/NodeGraphicsLink.h>

namespace game {
    class Scene;

    using NodeHR = engine::HierarchyRaw<engine::Node>;
    using NodePtr = engine::ref_ptr<NodeHR>;

    class Map {
    public:
        Map(engine::ref_ptr<Scene> scene);
        ~Map();
        void update(const float delta);

    private:
        void makeMapNode();
        engine::ref_ptr<Scene> _scene;
        NodePtr _mapNode;
    };
}