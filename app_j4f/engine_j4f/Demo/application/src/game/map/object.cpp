#include "object.h"

#include <Engine/Core/Threads/WorkersCommutator.h>

namespace game {

    void MapObject::updateTransform() noexcept {
        if (!_node || !_transformDirty) return;
        _transformDirty = false;

        using namespace engine;
        mat4f transform = makeMatrix(_scale);
        rotateMatrix_byOrder(transform, _rotation, _rotationsOrder);
        translateMatrixTo(transform, _position);

        auto && engineInstance = Engine::getInstance();

        engineInstance.getModule<WorkerThreadsCommutator>().enqueue(
                engineInstance.getThreadCommutationId(engine::Engine::Workers::RENDER_THREAD),
                [this, transform = std::move(transform)]() {
                    _node->value().setLocalMatrix(transform);
                });
    }
}