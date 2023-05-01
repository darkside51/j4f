#pragma once

#include "../../Core/Math/mathematic.h"
#include <cstdint>
#include <vector>

namespace engine {

    struct Particle {
        float time = 0.0f;
        float mass = 0.0f;
        uint32_t color = 0u;
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 size;
        glm::vec3 uv;
        glm::vec3 speed; // direction packed there
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
        glm::vec3 _position;
        glm::vec3 _rotation;
        std::vector<Particle> _particles;
    };
}