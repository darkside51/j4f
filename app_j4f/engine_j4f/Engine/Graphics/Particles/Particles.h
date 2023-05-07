#pragma once

#include "../../Core/Math/mathematic.h"
#include <cstdint>
#include <vector>

namespace engine {

    struct Particle {
        float time = 0.0f;
        float mass = 0.0f;
        uint32_t color = 0u;
        vec3f position;
        vec3f rotation;
        vec3f size;
        vec3f uv;
        vec3f speed; // direction packed there
    };

    class Emmiter;

    class ParticleAnimator {
    public:
        using TargetType = Emmiter*;
        inline void updateAnimation(const float delta) {}
    };

    class Emmiter {
    public:

        bool requestAnimUpdate() const noexcept { return _requestAnimUpdate; }
        void completeAnimUpdate() noexcept { _requestAnimUpdate = false; }
        void applyFrame(ParticleAnimator* animTree);

    private:
        bool _requestAnimUpdate = false;
        vec3f _position;
        vec3f _rotation;
        std::vector<Particle> _particles;
    };
}