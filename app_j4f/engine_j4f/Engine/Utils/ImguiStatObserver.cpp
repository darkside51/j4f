#include "ImguiStatObserver.h"
#include "../Core/Engine.h"
#include "StringHelper.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Vulkan/vkRenderer.h"
#include <imgui.h>

namespace engine {

    ImguiStatObserver::ImguiStatObserver(const Location location) : _location(location) {
        if (auto &&stat = Engine::getInstance().getModule<Statistic>()) {
            stat->addObserver(this);
        }

        auto &&engineInstance = Engine::getInstance();
        auto &&renderer = engineInstance.getModule<Graphics>()->getRenderer();
        const char *gpuType = "";

        switch (renderer->getDevice()->gpuProperties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                gpuType = "other";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                gpuType = "integrated";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                gpuType = "discrete";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                gpuType = "virtual";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                gpuType = "cpu";
                break;
            default:
                break;
        }

        _gpuName = fmtString("gpu ({}): {}", gpuType, renderer->getDevice()->gpuProperties.deviceName);

        const auto engineVersion = engineInstance.version();
        const auto apiVersion = engineInstance.getModule<Graphics>()->config().render_api_version;
        const auto applicationVersion = engineInstance.applicationVersion();

        _versions = fmtString("application version {}.{}.{}\nengine version {}.{}.{}\ngpu api: {}, version {}.{}.{}",
                              applicationVersion->major, applicationVersion->minor, applicationVersion->patch,
                              engineVersion->major, engineVersion->minor, engineVersion->patch,
                              renderer->getName(), apiVersion->major, apiVersion->minor, apiVersion->patch
                              );
    }

    ImguiStatObserver::~ImguiStatObserver() {
        if (auto &&stat = Engine::getInstance().getModule<Statistic>()) {
            stat->removeObserver(this);
        }
    }

    void ImguiStatObserver::statisticUpdate(const Statistic *statistic) {
        const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        const auto time = fmt::localtime(now);

        _timeString = fmtString("system time: {:%H:%M:%S}", time);

        auto &&renderer = Engine::getInstance().getModule<Graphics>()->getRenderer();
        const bool vsync = Engine::getInstance().getModule<Graphics>()->config().v_sync;

        auto [width, height] = Engine::getInstance().getModule<Graphics>()->getSize();

        _statString = fmtString("resolution: {}x{}\nv_sync: {}\ndraw calls: {}\n"
                                "fps: {}\ncpu frame time: {:.3}\nspeed mult: {:.3}",
                                width, height, vsync ? "on" : "off",
                                statistic->drawCalls(), statistic->fps(), statistic->cpuFrameTime(),
                                Engine::getInstance().getGameTimeMultiply());
    }

    class ImGuiStyleColorChanger {
    public:
        template<typename... Args>
        inline ImGuiStyleColorChanger(Args &&... args) noexcept {
            ImGui::PushStyleColor(std::forward<Args>(args)...);
        }

        ~ImGuiStyleColorChanger() noexcept {
            ImGui::PopStyleColor();
        }
    };

    void ImguiStatObserver::draw() {
        ImGuiIO &io = ImGui::GetIO();
        ImGuiWindowFlags window_flags = /*ImGuiWindowFlags_NoDecoration |*/ ImGuiWindowFlags_AlwaysAutoResize
                                                                            | ImGuiWindowFlags_NoSavedSettings |
                                                                            ImGuiWindowFlags_NoFocusOnAppearing
                                                                            | ImGuiWindowFlags_NoNav;

        switch (_location) {
            case Location::top_left:
            case Location::top_right:
            case Location::bottom_left:
            case Location::bottom_right: {
                constexpr float PAD = 4.0f;
                const ImGuiViewport *viewport = ImGui::GetMainViewport();
                ImVec2 work_pos = viewport->WorkPos; // use work area to avoid menu-bar/task-bar, if any!
                ImVec2 work_size = viewport->WorkSize;
                ImVec2 window_pos, window_pos_pivot;
                window_pos.x = (static_cast<uint8_t>(_location) & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x +
                                                                                                           PAD);
                window_pos.y = (static_cast<uint8_t>(_location) & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y +
                                                                                                           PAD);
                window_pos_pivot.x = (static_cast<uint8_t>(_location) & 1) ? 1.0f : 0.0f;
                window_pos_pivot.y = (static_cast<uint8_t>(_location) & 2) ? 1.0f : 0.0f;
                ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
                window_flags |= ImGuiWindowFlags_NoMove;
            }
                break;
            case Location::center:
                ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            case Location::locked:
                window_flags |= ImGuiWindowFlags_NoMove;
                break;
            default:
                break;
        }

        const auto bgColor = IM_COL32(0, 0, 0, 100);

        ImGuiStyleColorChanger _1(ImGuiCol_Text, IM_COL32(200, 200, 200, 200));
        ImGuiStyleColorChanger _2(ImGuiCol_ButtonActive, IM_COL32(50, 50, 50, 200));
        ImGuiStyleColorChanger _3(ImGuiCol_ButtonHovered, IM_COL32(0, 0, 0, 200));
        ImGuiStyleColorChanger _4(ImGuiCol_WindowBg, bgColor);
        ImGuiStyleColorChanger _5(ImGuiCol_TitleBg, bgColor);
        ImGuiStyleColorChanger _6(ImGuiCol_TitleBgCollapsed, bgColor);
        ImGuiStyleColorChanger _7(ImGuiCol_TitleBgActive, bgColor);

#ifdef _DEBUG
        if (ImGui::Begin("info(debug):", nullptr, window_flags)) {
#else
            if (ImGui::Begin("j4f_statistic(release):", nullptr, window_flags)) {
#endif
            ImGui::Text(_timeString.c_str());
            ImGui::Separator();
            ImGui::Text(_versions.c_str());
            ImGui::Separator();
            ImGui::Text(_gpuName.c_str());
            ImGui::Separator();
            ImGui::Text(_statString.c_str());

            if (ImGui::BeginPopupContextWindow()) {
                if (ImGui::MenuItem("locked", nullptr, _location == Location::locked)) _location = Location::locked;
                if (ImGui::MenuItem("custom", nullptr, _location == Location::custom)) _location = Location::custom;
                if (ImGui::MenuItem("center", nullptr, _location == Location::center)) _location = Location::center;
                if (ImGui::MenuItem("top_left", nullptr, _location == Location::top_left))
                    _location = Location::top_left;
                if (ImGui::MenuItem("top_right", nullptr, _location == Location::top_right))
                    _location = Location::top_right;
                if (ImGui::MenuItem("bottom_left", nullptr, _location == Location::bottom_left))
                    _location = Location::bottom_left;
                if (ImGui::MenuItem("bottom_right", nullptr, _location == Location::bottom_right))
                    _location = Location::bottom_right;
                ImGui::EndPopup();
            }
        }

        ImGui::End();
    }
}