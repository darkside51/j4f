#include "units_manager.h"

#include <Engine/Graphics/Mesh/MeshLoader.h>

namespace game {

    UnitsManager::UnitsManager() {
        _units.reserve(10u);
    }
    UnitsManager::~UnitsManager() = default;

    Unit& UnitsManager::createUnit(std::string_view name) {
        return _units.emplace_back(name);
    }

    void UnitsManager::removeUnit() {
        //_units.erase(std::remove_if(_units.begin(), _units.end(), [object](const auto& obj){
        //    return obj.get() == object.get();
        //}), _objects.end());
    }

    void UnitsManager::update(const float delta, const engine::Camera& cam) {
        for (auto & unit : _units) {
            unit.update(delta, cam);
        }
    }
}