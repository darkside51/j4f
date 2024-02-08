#include "unit.h"

#include <Engine/Graphics/Mesh/AnimationTree.h>

namespace game {

    Unit::Unit() : Entity(this), _animations(nullptr) {}

    Unit::~Unit() = default;

    void update(const float delta) {}

}