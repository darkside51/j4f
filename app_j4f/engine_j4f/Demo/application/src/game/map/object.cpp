#include "object.h"

#include <Engine/Core/Threads/WorkersCommutator.h>
#include <Engine/Graphics/Texture/TextureHandler.h>

namespace game {

    MapObject::MapObject() : _name("") {}
    MapObject::MapObject(std::string && name) : _name(std::move(name)) {}
    MapObject::MapObject(const std::string & name) : _name(name) {}
    MapObject::MapObject(std::string_view name) : _name(name) {}
    MapObject::~MapObject() = default;

    const engine::mat4f& MapObject::getTransform() const noexcept {
        return _node->value().localMatrix();
    }

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

    void MapObject::addTexture(const engine::TexturePtr & t) { _textures.push_back(t); }
}