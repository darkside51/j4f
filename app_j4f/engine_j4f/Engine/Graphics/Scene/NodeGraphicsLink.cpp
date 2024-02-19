#include "NodeGraphicsLink.h"
#include "../Render/RenderedEntity.h"

namespace engine {
    RenderDescriptor& RenderObject::getRenderDescriptor() noexcept {
        return _renderEntity->getRenderDescriptor();
    }
}