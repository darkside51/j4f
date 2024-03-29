#pragma once

#include "object.h"

#include <memory>
#include <vector>

namespace game {
    class Scene;

    class Map {
    public:
        Map();
        ~Map();

        /*void addObject(std::unique_ptr<MapObject> && object) {
            _objects.emplace_back(std::move(object));
        }

        void removeObject(engine::ref_ptr<MapObject> object) {
            _objects.erase(std::remove_if(_objects.begin(), _objects.end(), [object](const auto& obj){
                return obj.get() == object.get();
            }), _objects.end());
        }*/

        void update(const float delta);

    private:
        void makeMapNode();
        NodePtr _mapNode;
    };
}