#pragma once

#include "../../Render/RenderedEntity.h"
#include "../../Render/RenderHelper.h"
#include "../../../Input/Input.h"

#include <vector>
#include <cstdint>
#include <utility>
#include <unordered_map>

namespace engine {

    class ImguiGraphics : public RenderedEntity, public IRenderDescriptorCustomRenderer {
    public:
        ImguiGraphics();
        ~ImguiGraphics() override;

        void updateRenderData(const glm::mat4& /*worldMatrix*/, const bool /*worldMatrixChanged*/);
        inline void updateModelMatrixChanged(const bool /*worldMatrixChanged*/) noexcept { }

        void update(const float delta);
        void render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const glm::mat4* cameraMatrix) override;

        bool onInputPointerEvent(const PointerEvent& event);
        bool onInputWheelEvent(const float dx, const float dy);
        bool onInpuKeyEvent(const KeyEvent& event);

    private:
        void createRenderData();
        void createFontTexture();
        void destroyFontTexture();
        void setupKeyMap();
        void renderGUI();

        std::vector<VulkanStreamBuffer> _dynamic_vertices;
        std::vector<VulkanStreamBuffer> _dynamic_indices;
        vulkan::VulkanGpuProgram* _program = nullptr;
        vulkan::VulkanTexture* _fontTexture = nullptr;
        std::unordered_map<uint8_t, std::pair<uint8_t, uint8_t>> _keyMap;
    };

}