#include "ImGuiCameraInfo.h"
#include "../Graphics/Camera.h"

#include <imgui.h>

namespace engine {

    void ImGuiCameraInfo::draw(const Camera &camera, std::string_view label) {
        auto const cameraPadding = camera.getPadding();
        ImGuiIO const &io = ImGui::GetIO();

        if (!label.empty()) {
            ImGui::GetBackgroundDrawList()->AddText({cameraPadding.x, io.DisplaySize.y - cameraPadding.y},
                                                    IM_COL32_WHITE,
                                                    label.data());
        }

        if (cameraPadding.x > 0.0f) {
            ImGui::GetBackgroundDrawList()->AddLine(
                    ImVec2(cameraPadding.x / io.DisplayFramebufferScale.x,
                           io.DisplaySize.y - cameraPadding.y / io.DisplayFramebufferScale.y),
                    ImVec2(cameraPadding.x / io.DisplayFramebufferScale.x, 0.0f),
                    ImGui::GetColorU32(ImVec4(0.0f, 1.0f, 0.0f, 1.0f)), 1.0f);
        }

        if (cameraPadding.y > 0.0f) {
            ImGui::GetBackgroundDrawList()->AddLine(
                    ImVec2(cameraPadding.x / io.DisplayFramebufferScale.x,
                           io.DisplaySize.y - cameraPadding.y / io.DisplayFramebufferScale.y),
                    ImVec2(io.DisplaySize.x, io.DisplaySize.y - cameraPadding.y / io.DisplayFramebufferScale.y),
                    ImGui::GetColorU32(ImVec4(1.0f, 0.0f, 0.0f, 1.0f)), 1.0f);
        }
    }

}