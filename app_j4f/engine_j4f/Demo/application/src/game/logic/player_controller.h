#pragma once

#include <Engine/Core/ref_ptr.h>
#include <Engine/Input/Input.h>

namespace game {
    class Unit;

    class PlayerController {
    public:
        void assign(const Unit & unit);

        void onPointerEvent(const engine::PointerEvent &event);
    private:
        engine::ref_ptr<Unit> _unit;
    };
}