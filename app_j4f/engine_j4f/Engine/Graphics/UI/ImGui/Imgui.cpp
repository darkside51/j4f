#include "Imgui.h"
#include <imgui.h>
#include "../../GpuProgramsManager.h"

namespace engine {

    ImguiGraphics::ImguiGraphics() {
        ImGui::CreateContext();
        createRenderData();
        createFontTexture();
        setupKeyMap();

        ImGuiIO& io = ImGui::GetIO();
        io.DisplayFramebufferScale = ImVec2(1, 1);
    }

    ImguiGraphics::~ImguiGraphics() {
        ImGui::DestroyContext();
        destroyFontTexture();
        _dynamic_vertices.clear();
        _dynamic_indices.clear();
    }

    void ImguiGraphics::createRenderData() {
        _renderDescriptor.mode = RenderDescritorMode::CUSTOM_DRAW;
        _renderDescriptor.customRenderer = this;

        _renderState.vertexDescription.bindings_strides.emplace_back(0, sizeof(ImDrawVert));
        _renderState.topology = { vulkan::PrimitiveTopology::TRIANGLE_LIST, false };
        _renderState.rasterizationState = vulkan::VulkanRasterizationState(vulkan::CullMode::CULL_MODE_NONE, vulkan::PoligonMode::POLYGON_MODE_FILL);
        _renderState.blendMode = vulkan::CommonBlendModes::blend_alpha;
        _renderState.depthState = vulkan::VulkanDepthState(false, false, VK_COMPARE_OP_LESS);
        _renderState.stencilState = vulkan::VulkanStencilState(false);

        _vertexInputAttributes = []() -> std::vector<VkVertexInputAttributeDescription>{
            std::vector<VkVertexInputAttributeDescription> vertexInputAttributs(3);
            // attribute location 0: position
            vertexInputAttributs[0].binding = 0;
            vertexInputAttributs[0].location = 0;
            vertexInputAttributs[0].format = VK_FORMAT_R32G32_SFLOAT;
            vertexInputAttributs[0].offset = offset_of(ImDrawVert, pos);
            // attribute location 1: uv
            vertexInputAttributs[1].binding = 0;
            vertexInputAttributs[1].location = 1;
            vertexInputAttributs[1].format = VK_FORMAT_R32G32_SFLOAT;
            vertexInputAttributs[1].offset = offset_of(ImDrawVert, uv);
            // attribute location 2: color
            vertexInputAttributs[2].binding = 0;
            vertexInputAttributs[2].location = 2;
            vertexInputAttributs[2].format = VK_FORMAT_R8G8B8A8_UNORM;
            vertexInputAttributs[2].offset = offset_of(ImDrawVert, col);

            return vertexInputAttributs;
        }();

        _renderState.vertexDescription.attributesCount = static_cast<uint32_t>(_vertexInputAttributes.size());
        _renderState.vertexDescription.attributes = _vertexInputAttributes.data();

        _renderDescriptor.renderDataCount = 1;
        _renderDescriptor.renderData = new vulkan::RenderData *[1];
        _renderDescriptor.renderData[0] = new vulkan::RenderData();
        _renderDescriptor.renderData[0]->indexType = VK_INDEX_TYPE_UINT16;

        auto&& gpuProgramManager = Engine::getInstance().getModule<Graphics>()->getGpuProgramsManager();

        std::vector<engine::ProgramStageInfo> psiCTextured;
        psiCTextured.emplace_back(ProgramStage::VERTEX, "resources/shaders/imgui.vsh.spv");
        psiCTextured.emplace_back(ProgramStage::FRAGMENT, "resources/shaders/imgui.psh.spv");
        _program = reinterpret_cast<vulkan::VulkanGpuProgram*>(gpuProgramManager->getProgram(psiCTextured));

        // fixed gpu layout works
        _fixedGpuLayouts.resize(3);
        _fixedGpuLayouts[0].second = "uScale";
        _fixedGpuLayouts[1].second = "uTranslate";
        _fixedGpuLayouts[2].second = "sTexture";

        setPipeline(Engine::getInstance().getModule<Graphics>()->getRenderer()->getGraphicsPipeline(_renderState, _program));
    }

    void ImguiGraphics::createFontTexture() {
        ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels = nullptr;
        int width = 0;
        int height = 0;
        io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

        uint8_t* uploadData = static_cast<uint8_t*>(malloc(static_cast<size_t>(width * height * 4)));
        uint8_t* dstPtr = uploadData;
        uint8_t* srcPtr = pixels;

        uint32_t totalSize = height * width;
        for (uint32_t i = 0; i < totalSize; ++i) {
            dstPtr[0] = *srcPtr;
            dstPtr[1] = *srcPtr;
            dstPtr[2] = *srcPtr;
            dstPtr[3] = *srcPtr;

            srcPtr += 1;
            dstPtr += 4;
        }

        auto&& renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();
        _fontTexture = new vulkan::VulkanTexture(renderer, width, height,1);
        _fontTexture->setSampler(renderer->getSampler(
                VK_FILTER_LINEAR,
                VK_FILTER_LINEAR,
                VK_SAMPLER_MIPMAP_MODE_NEAREST,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK
        ));

        const void* imageData = &uploadData[0];
        _fontTexture->create(&imageData, 1, VK_FORMAT_R8G8B8A8_UNORM, 32, false,
                             false, VK_IMAGE_VIEW_TYPE_2D);
        _fontTexture->createSingleDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0);
        free(uploadData);

        io.Fonts->SetTexID(_fontTexture);
    }

    void ImguiGraphics::destroyFontTexture() {
        if (_fontTexture) {
            delete _fontTexture;
            _fontTexture = nullptr;
        }
    }

    void ImguiGraphics::updateRenderData(const glm::mat4& /*worldMatrix*/, const bool /*worldMatrixChanged*/) { }

    void ImguiGraphics::update(const float delta) {
        ImGuiIO& io = ImGui::GetIO();
        io.DeltaTime = (delta > 0.0) ? delta : (1.0f / 60.0f);

        const auto [width, height] = Engine::getInstance().getModule<Graphics>()->getSize();
        io.DisplaySize = ImVec2(width, height);
        ImGui::NewFrame();
    }

    void ImguiGraphics::render(vulkan::VulkanCommandBuffer& commandBuffer, const uint32_t currentFrame, const glm::mat4*) {
        const uint8_t swapChainImagesCount = Engine::getInstance().getModule<Graphics>()->getRenderer()->getSwapchainImagesCount();

        if (swapChainImagesCount != _dynamic_vertices.size()) {
            _dynamic_vertices.resize(swapChainImagesCount);
            _dynamic_indices.resize(swapChainImagesCount);
        }

        _dynamic_vertices[currentFrame].recreate(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        _dynamic_indices[currentFrame].recreate(VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

        ImGui::Render();

        const ImDrawData* drawData = ImGui::GetDrawData();
        std::array scale{ 2.0f / drawData->DisplaySize.x, 2.0f / drawData->DisplaySize.y };
        std::array translate{ (drawData->DisplayPos.x - drawData->DisplaySize.x * 0.5f) * scale[0],
                              (drawData->DisplayPos.y - drawData->DisplaySize.y * 0.5f) * scale[1] };

        setParamForLayout(_fixedGpuLayouts[0].first, scale.data(),true, 2);
        setParamForLayout(_fixedGpuLayouts[1].first, translate.data(),true, 2);

        const auto currentScissor = commandBuffer.getCurrentScissor();

        size_t vOffset;
        size_t iOffset;

        auto &renderData = _renderDescriptor.renderData[0];

        for (int i = 0; i < drawData->CmdListsCount; ++i) {
            const ImDrawList* drawList = drawData->CmdLists[i];
            const uint32_t vertexBufferSize = drawList->VtxBuffer.size() * sizeof(ImDrawVert);
            const uint32_t indexBufferSize = drawList->IdxBuffer.size() * sizeof(ImDrawIdx);

            auto&& vBuffer =_dynamic_vertices[currentFrame].addData(drawList->VtxBuffer.Data, vertexBufferSize, vOffset);
            auto&& iBuffer =_dynamic_indices[currentFrame].addData(drawList->IdxBuffer.Data,indexBufferSize,iOffset);

            renderData->vertexes = &vBuffer;
            renderData->indexes = &iBuffer;

            uint32_t indexBufferOffset = 0;
            const uint32_t firstIndex = iOffset / sizeof(ImDrawIdx);

            for (int j = 0; j < drawList->CmdBuffer.Size; ++j) {
                ImDrawCmd const & cmd = drawList->CmdBuffer[j];
                commandBuffer.cmdSetScissor(cmd.ClipRect.x, cmd.ClipRect.y, cmd.ClipRect.z - cmd.ClipRect.x, cmd.ClipRect.w - cmd.ClipRect.y);

                auto* texture = static_cast<vulkan::VulkanTexture*>(cmd.TextureId);
                setParamForLayout(_fixedGpuLayouts[2].first, texture,false);

                vulkan::RenderData::RenderPart renderPart{
                        firstIndex,	                                        // firstIndex
                        static_cast<uint32_t>(cmd.ElemCount),		// indexCount
                        0,										// vertexCount (parameter no used with indexed render)
                        0,										// firstVertex
                        1,										// instanceCount (can change later)
                        0,										// firstInstance (can change later)
                        vOffset,									// vbOffset
                        indexBufferOffset							// ibOffset
                };

                renderData->setRenderParts(&renderPart, 1);
                renderData->prepareRender();
                renderData->render(commandBuffer, currentFrame);
                renderData->setRenderParts(nullptr, 0);

                indexBufferOffset += cmd.ElemCount * sizeof(ImDrawIdx);
            }
        }

        if (currentScissor.has_value()) {
            commandBuffer.cmdSetScissor(currentScissor.value());
        } else {
            const auto [width, height] = Engine::getInstance().getModule<Graphics>()->getSize();
            commandBuffer.cmdSetScissor(0, 0, width, height);
        }
    }

    void ImguiGraphics::setupKeyMap() {
        ImGuiIO& io = ImGui::GetIO();
        io.KeyMap[ImGuiKey_LeftArrow] = static_cast<uint8_t>(KeyboardKey::K_LEFT);
        io.KeyMap[ImGuiKey_RightArrow] = static_cast<uint8_t>(KeyboardKey::K_RIGHT);
        io.KeyMap[ImGuiKey_UpArrow] = static_cast<uint8_t>(KeyboardKey::K_UP);
        io.KeyMap[ImGuiKey_DownArrow] = static_cast<uint8_t>(KeyboardKey::K_DOWN);
        io.KeyMap[ImGuiKey_Delete] = static_cast<uint8_t>(KeyboardKey::K_DELETE);
        io.KeyMap[ImGuiKey_Backspace] = static_cast<uint8_t>(KeyboardKey::K_BACKSPACE);
        io.KeyMap[ImGuiKey_Space] = static_cast<uint8_t>(KeyboardKey::K_SPACE);
        io.KeyMap[ImGuiKey_Enter] = static_cast<uint8_t>(KeyboardKey::K_ENTER);

        _keyMap[static_cast<uint8_t>(KeyboardKey::K_BACKSPACE)] = {8, 8};
        _keyMap[static_cast<uint8_t>(KeyboardKey::K_TAB)] = {9, 9};
        _keyMap[static_cast<uint8_t>(KeyboardKey::K_SPACE)] = {32, 32};
        _keyMap[static_cast<uint8_t>(KeyboardKey::K_LEFT)] = {17, 17};
        _keyMap[static_cast<uint8_t>(KeyboardKey::K_RIGHT)] = {18, 18};
        _keyMap[static_cast<uint8_t>(KeyboardKey::K_UP)] = {19, 19};
        _keyMap[static_cast<uint8_t>(KeyboardKey::K_DOWN)] = {20, 20};
        _keyMap[static_cast<uint8_t>(KeyboardKey::K_MINUS)] = {45, 95};
        _keyMap[static_cast<uint8_t>(KeyboardKey::K_SLASH)] = {47, 47};
        _keyMap[static_cast<uint8_t>(KeyboardKey::K_0)] = {48, 41};
        _keyMap[static_cast<uint8_t>(KeyboardKey::K_1)] = {49, 33};
        _keyMap[static_cast<uint8_t>(KeyboardKey::K_2)] = {50, 64};
        _keyMap[static_cast<uint8_t>(KeyboardKey::K_3)] = {51, 35};
        _keyMap[static_cast<uint8_t>(KeyboardKey::K_4)] = {52, 36};
        _keyMap[static_cast<uint8_t>(KeyboardKey::K_5)] = {53, 37};
        _keyMap[static_cast<uint8_t>(KeyboardKey::K_6)] = {54, 94};
        _keyMap[static_cast<uint8_t>(KeyboardKey::K_7)] = {55, 38};
        _keyMap[static_cast<uint8_t>(KeyboardKey::K_8)] = {56, 42};
        _keyMap[static_cast<uint8_t>(KeyboardKey::K_9)] = {57, 40};
        _keyMap[static_cast<uint8_t>(KeyboardKey::K_EQUAL)] = {61, 43};

        for (uint8_t k = static_cast<uint8_t>(KeyboardKey::K_A); k <= static_cast<uint8_t>(KeyboardKey::K_Z); ++k) {
            uint8_t i = k - static_cast<uint8_t>(KeyboardKey::K_A);
            _keyMap[k] = {97 + i, 65 + i};
        }

        _keyMap[static_cast<uint8_t>(KeyboardKey::K_DELETE)] = {127, 127};
    }

    bool ImguiGraphics::onInputPointerEvent(const PointerEvent& event) {
        ImGuiIO& io = ImGui::GetIO();

        switch (event.state) {
            case InputEventState::IES_PRESS:
                io.AddMouseButtonEvent(static_cast<int>(event.button), true);
                break;
            case InputEventState::IES_RELEASE:
                io.AddMouseButtonEvent(static_cast<int>(event.button), false);
                break;
            case InputEventState::IES_NONE:
            {
                io.MousePos = ImVec2(event.x, io.DisplaySize.y - event.y);
            }
                break;
            default:
                break;
        }

        return io.WantCaptureMouse;
    }

    bool ImguiGraphics::onInputWheelEvent(const float dx, const float dy) {
        ImGuiIO& io = ImGui::GetIO();
        io.AddMouseWheelEvent(dx, dy);
        return io.WantCaptureMouse;
    }

    bool ImguiGraphics::onInpuKeyEvent(const KeyEvent& event) {
        ImGuiIO& io = ImGui::GetIO();
        io.KeyShift = Engine::getInstance().getModule<Input>()->isShiftPressed();
        io.KeyAlt = Engine::getInstance().getModule<Input>()->isAltPressed();
        io.KeySuper = Engine::getInstance().getModule<Input>()->isSuperPressed();
        io.KeyCtrl = Engine::getInstance().getModule<Input>()->isCtrlPressed();
        io.KeysDown[static_cast<uint8_t>(event.key)] = event.state == InputEventState::IES_PRESS;

        if (event.state == InputEventState::IES_PRESS) {
            const auto& [key, keyWithShift] = _keyMap[static_cast<uint8_t>(event.key)];
            io.AddInputCharacter(io.KeyShift ? keyWithShift: key);
        }

        return io.WantCaptureKeyboard;
    }
}