#include "object.h"

namespace game {

    void MapObject::updateNodeTransform() noexcept {
        if (!_node || !_transformDirty) return;
        _transformDirty = false;

        // update _node transformation
    }

}