#pragma once

#include <Engine/Core/ref_ptr.h>
#include <Engine/Core/Math/mathematic.h>
#include <Engine/Input/Input.h>
#include <Engine/Events/Bus.h>

#include <cstdint>

namespace game {
    class Unit;

class PlayerController : public engine::EventObserverImpl<uint8_t> {
    public:
        PlayerController();
        ~PlayerController();
        void assign(const Unit & unit);

        void onPointerEvent(const engine::PointerEvent &event);
        bool onKeyEvent(const engine::KeyEvent& event);
        bool processEvent(const uint8_t & id) override;
        bool processEvent(uint8_t&& id) override { return processEvent(id); }

        engine::vec3f getPlayerPosition() const;
    private:
        engine::ref_ptr<Unit> _unit;
    };
}