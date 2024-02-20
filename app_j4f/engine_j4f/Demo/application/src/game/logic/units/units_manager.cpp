#include "units_manager.h"

#include <Engine/Graphics/Mesh/MeshLoader.h>

namespace game {

    UnitsManager::UnitsManager() :
    _meshGraphicsBuffer(std::make_unique<engine::MeshGraphicsDataBuffer>(10 * 1024 * 1024, 10 * 1024 * 1024)) {

    }

    UnitsManager::~UnitsManager() = default;

    Unit& UnitsManager::createUnit() {
        return _units.emplace_back(engine::make_ref(_meshGraphicsBuffer));
    }

    void UnitsManager::removeUnit() {
        //_units.erase(std::remove_if(_units.begin(), _units.end(), [object](const auto& obj){
        //    return obj.get() == object.get();
        //}), _objects.end());
    }

    void UnitsManager::update(const float delta) {
        for (auto & unit : _units) {
            unit.update(delta);
        }
    }

    engine::ref_ptr<engine::MeshGraphicsDataBuffer> UnitsManager::getGraphicsBuffer() {
        return engine::make_ref(_meshGraphicsBuffer);
    }
}