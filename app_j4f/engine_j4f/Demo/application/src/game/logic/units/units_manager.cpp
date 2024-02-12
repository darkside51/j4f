#include "units_manager.h"

namespace game {

    Unit& UnitsManager::createUnit() {
        return _units.emplace_back();
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
}