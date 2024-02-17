#include "ImguiStatObserver.h"
#include "../Core/Engine.h"
#include "StringHelper.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Vulkan/vkRenderer.h"
#include "HardwareInfo.h"
#include <imgui.h>

namespace engine {

    ImguiStatObserver::ImguiStatObserver(const Location location) : _location(location) {
        auto &&stat = Engine::getInstance().getModule<Statistic>();
        stat.addObserver(this);

#if defined j4f_PLATFORM_LINUX
        _os = "linux";
#elif defined j4f_PLATFORM_WINDOWS
        _os = "windows";
#endif

        auto &&engineInstance = Engine::getInstance();
        auto &&renderer = engineInstance.getModule<Graphics>().getRenderer();

        CPUInfo cpuInfo;
        _cpuName = fmtString("cpu: {}", cpuInfo.model());

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

        constexpr uint32_t toGB = 1 << 30u; // 1024 * 1024 * 1024;
        _ram = fmtString("ram: {:.2f} GB", static_cast<double>(getTotalSystemMemory()) / toGB);

        const auto engineVersion = engineInstance.version();
        const auto apiVersion = engineInstance.getModule<Graphics>().config().render_api_version;
        const auto applicationVersion = engineInstance.applicationVersion();

        _versions = fmtString("application version {}\nengine version {}\ngpu api: {}, version {}",
                              applicationVersion.str().c_str(),
                              engineVersion.str().c_str(),
                              renderer->name(), apiVersion.str().c_str()
                              );

        memset(_renderFps_array.data(), 0, sizeof(float) * _renderFps_array.size());
        memset(_updateFps_array.data(), 0, sizeof(float) * _updateFps_array.size());
    }

    ImguiStatObserver::~ImguiStatObserver() {
        Engine::getInstance().getModule<Statistic>().removeObserver(this);
    }

    void ImguiStatObserver::statisticUpdate(const Statistic *statistic) {
        const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        const auto time = fmt::localtime(now);

        _timeString = fmtString("time: {:%H:%M:%S}", time);

        auto && graphics = Engine::getInstance().getModule<Graphics>();

        auto &&renderer = graphics.getRenderer();
        const bool vsync = graphics.config().v_sync;

        auto [width, height] = graphics.getSize();

        _statString = fmtString("resolution: {}x{}\nv_sync: {}\ndraw calls: {}\n"
                                "cpu frame time: {:.5f}\nspeed mult: {:.3}",
                                width, height, vsync ? "on" : "off",
                                statistic->drawCalls(), statistic->cpuFrameTime(),
                                Engine::getInstance().getTimeMultiply());

        auto const renderFps = statistic->renderFps();
        _renderFps_array[_fps_array_idx] = renderFps;
        _maxRenderFps = std::max(_maxRenderFps, static_cast<float>(renderFps));
        _renderFps = fmtString("render: {} fps", renderFps);

        auto const updateFps = statistic->updateFps();
        _updateFps_array[_fps_array_idx] = updateFps;
        _maxUpdateFps = std::max(_maxUpdateFps, static_cast<float>(updateFps));
        _updateFps = fmtString("update: {} fps", updateFps);

        if (++_fps_array_idx == _renderFps_array.size()) { _fps_array_idx = 0; }
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
        ImGuiWindowFlags window_flags = /*ImGuiWindowFlags_NoDecoration |*/ ImGuiWindowFlags_AlwaysAutoResize
                                                                            | ImGuiWindowFlags_NoSavedSettings
                                                                            | ImGuiWindowFlags_NoFocusOnAppearing
                                                                            | ImGuiWindowFlags_NoNav;

        switch (_location) {
            case Location::top_left:
            case Location::top_right:
            case Location::bottom_left:
            case Location::bottom_right: {
                constexpr float pad = 4.0f;
                const ImGuiViewport *viewport = ImGui::GetMainViewport();
                ImVec2 work_pos = viewport->WorkPos; // use work area to avoid menu-bar/task-bar, if any!
                ImVec2 work_size = viewport->WorkSize;
                ImVec2 window_pos, window_pos_pivot;
                window_pos.x = (static_cast<uint8_t>(_location) & 1) ? (work_pos.x + work_size.x - pad) : (work_pos.x +
                        pad);
                window_pos.y = (static_cast<uint8_t>(_location) & 2) ? (work_pos.y + work_size.y - pad) : (work_pos.y +
                        pad);
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

        const std::array<ImGuiStyleColorChanger, 11u> changedColors = {
                ImGuiStyleColorChanger{ImGuiCol_Text, IM_COL32(200, 200, 200, 200)},
                {ImGuiCol_ButtonActive, IM_COL32(50, 50, 50, 200)},
                {ImGuiCol_ButtonHovered, IM_COL32(0, 0, 0, 200)},
                {ImGuiCol_WindowBg, bgColor},
                {ImGuiCol_TitleBg, bgColor},
                {ImGuiCol_TitleBgCollapsed, bgColor},
                {ImGuiCol_TitleBgActive, bgColor},
                {ImGuiCol_HeaderActive, IM_COL32(50, 50, 50, 200)},
                {ImGuiCol_HeaderHovered, IM_COL32(0, 0, 0, 200)},
                {ImGuiCol_PlotLines, IM_COL32(0, 0, 0, 200)},
                {ImGuiCol_FrameBg, IM_COL32(200, 200, 200, 100)}
        };

#ifdef _DEBUG
        constexpr char* kBuildType = "info(debug):";
#else
        constexpr char* kBuildType = "info(release):";
#endif
        if (ImGui::Begin(kBuildType, nullptr, window_flags)) {
            ImGui::Text(_timeString.c_str());

            if (ImGui::TreeNodeEx("platform", ImGuiTreeNodeFlags_OpenOnArrow)) {
                ImGui::Text(_os.c_str());
                ImGui::TreePop();
            }
            ImGui::Separator();
            if (ImGui::TreeNodeEx("versions", ImGuiTreeNodeFlags_OpenOnArrow)) {
                ImGui::Text(_versions.c_str());
                ImGui::TreePop();
            }
            ImGui::Separator();
            if (ImGui::TreeNodeEx("hardware info", ImGuiTreeNodeFlags_OpenOnArrow)) {
                ImGui::BulletText(_cpuName.c_str());
                ImGui::BulletText(_gpuName.c_str());
                ImGui::BulletText(_ram.c_str());
                ImGui::TreePop();
            }
            ImGui::Separator();
            if (ImGui::TreeNodeEx("frame info", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text(_statString.c_str());

                ImGui::Separator();
                ImGui::BulletText(_renderFps.c_str());
//                ImGui::PlotLines("", _renderFps_array.data(), _renderFps_array.size(),
//                                 _fps_array_idx, NULL, 0.0, _maxRenderFps,
//                                 ImVec2(0, 30.0f));
                ImGui::PlotHistogram("", _renderFps_array.data(), _renderFps_array.size(),
                                     _fps_array_idx, NULL, 0.0, _maxRenderFps, ImVec2(0, 30.0f));

                ImGui::BulletText(_updateFps.c_str());
//                ImGui::PlotLines("", _updateFps_array.data(), _updateFps_array.size(),
//                                 _fps_array_idx, NULL, 0.0, _maxUpdateFps,
//                                 ImVec2(0, 30.0f));
                ImGui::PlotHistogram("", _updateFps_array.data(), _updateFps_array.size(),
                                     _fps_array_idx, NULL, 0.0, _maxUpdateFps, ImVec2(0, 30.0f));



                ImGui::TreePop();
            }

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