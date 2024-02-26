#pragma once

#include <Engine/Core/ref_ptr.h>
#include <Engine/Core/Hierarchy.h>
#include <Engine/Graphics/Scene/NodeGraphicsLink.h>

namespace engine {
    class Node;
}

namespace game {
    template<typename T>
    using NodeRenderer = engine::NodeRendererImpl<T>;

    using NodeHR = engine::HierarchyRaw<engine::Node>;
    using NodePtr = engine::ref_ptr<NodeHR>;
}