#pragma once

#include <string_view>

namespace engine {
    class Camera;

    class ImGuiCameraInfo {
    public:
        void draw(const Camera& camera, std::string_view label);
    };

}
