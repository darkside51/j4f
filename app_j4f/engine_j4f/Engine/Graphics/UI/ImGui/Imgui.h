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
        friend class Graphics;
        friend void generateShaders(const CancellationToken& token, ImguiGraphics* g);
    public:
        inline static ImguiGraphics* getInstance() noexcept {
            static ImguiGraphics* imgui = new ImguiGraphics();
            return imgui;
        }

        ~ImguiGraphics() override;

        void updateRenderData(const mat4f& /*worldMatrix*/, const bool /*worldMatrixChanged*/);
        inline void updateModelMatrixChanged(const bool /*worldMatrixChanged*/) noexcept { }

        void update(const float delta);
        void render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const ViewParams& viewParams, const uint16_t drawCount = 1u) override;

        bool onInputPointerEvent(const PointerEvent& event);
        bool onInputWheelEvent(const float dx, const float dy);
        bool onInpuKeyEvent(const KeyEvent& event);

        inline static std::vector<unsigned int> imguiVsh = {};
        inline static std::vector<unsigned int> imguiPsh = {};

    private:
        ImguiGraphics();
        void createRenderData();
        void createFontTexture();
        void destroyFontTexture();
        void setupKeyMap();
        void destroy();

        std::vector<VulkanStreamBuffer> _dynamic_vertices;
        std::vector<VulkanStreamBuffer> _dynamic_indices;
        vulkan::VulkanGpuProgram* _program = nullptr;
        vulkan::VulkanTexture* _fontTexture = nullptr;
        std::unordered_map<uint8_t, std::pair<uint8_t, uint8_t>> _keyMap;
        bool _initComplete = false;
    };

}