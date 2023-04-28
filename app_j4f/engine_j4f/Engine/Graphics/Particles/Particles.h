#pragma once

#include "../../Core/Math/mathematic.h"

namespace engine {

    struct Particle {
        float time = 0.0f;
        uint32_t color = 0u;
        glm::vec3 position;
        glm::vec3 uv;
        glm::vec3 speed; // direction packed there
    };

    class Emmiter {
    public:
    private:
    };
}