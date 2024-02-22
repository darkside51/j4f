#include "main_screen.h"

#include <Engine/Utils/StringHelper.h>

#include <imgui.h>
#include <array>

namespace game {

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

	void MainScreen::draw(const float delta) {
		//ImGui::ShowDemoWindow();

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
			| ImGuiWindowFlags_NoSavedSettings
			| ImGuiWindowFlags_NoFocusOnAppearing
			| ImGuiWindowFlags_NoNav
			| ImGuiWindowFlags_NoMove;

		constexpr float kWindowRound = 20.0f;
		constexpr float kFrameRound = 10.0f;
		const float height = 70.0f;
		const float btnSize = height - 10.0f;
		const uint8_t btnCount = 10u;
		const float btnSpace = 5.0f;
		const float width = (btnSize + btnSpace) * btnCount - btnSpace + 2 * (kWindowRound - kFrameRound);
		const float hspace = 10.0f;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		const ImVec2 work_size = viewport->WorkSize;

		ImVec2 window_pos;
		window_pos.x =  (work_size.x - width) * 0.5f;
		window_pos.y = work_size.y - (height + hspace);
		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);

		ImGuiStyle& style = ImGui::GetStyle();
		auto windowRounding = kWindowRound;
		auto frameRounding = kFrameRound;
		std::swap(style.WindowRounding, windowRounding);
		std::swap(style.FrameRounding, frameRounding);

		const auto bgColor = IM_COL32(0, 0, 0, 100);

		const std::array<ImGuiStyleColorChanger, 14u> changedColors = {
				ImGuiStyleColorChanger{ImGuiCol_Text, IM_COL32(200, 200, 200, 200)},
				{ImGuiCol_Button, IM_COL32(50, 50, 50, 200)},
				{ImGuiCol_ButtonActive, IM_COL32(75, 75, 75, 200)},
				{ImGuiCol_ButtonHovered, IM_COL32(0, 0, 0, 200)},
				{ImGuiCol_WindowBg, bgColor},
				{ImGuiCol_TitleBg, bgColor},
				{ImGuiCol_TitleBgCollapsed, bgColor},
				{ImGuiCol_TitleBgActive, bgColor},
				{ImGuiCol_HeaderActive, IM_COL32(50, 50, 50, 200)},
				{ImGuiCol_HeaderHovered, IM_COL32(0, 0, 0, 200)},
				{ImGuiCol_PlotLines, IM_COL32(0, 0, 0, 200)},
				{ImGuiCol_FrameBg, IM_COL32(200, 200, 200, 100)},
				{ImGuiCol_PlotHistogram, IM_COL32(60, 60, 60, 200)},
				{ImGuiCol_PlotHistogramHovered, IM_COL32(0, 0, 0, 200)},
		};

		if (ImGui::Begin("##skills_panel", nullptr, window_flags)) {
			
			for (uint8_t i = 0u; i < btnCount; ++i) {
				ImGui::SetCursorPos({ (kWindowRound - kFrameRound) + (btnSize + btnSpace) * i, 5.0f });
				if (ImGui::Button(engine::fmt_string("#act_{}", i), { btnSize, btnSize })) {
					//
				}
			}

			ImGui::End();
		}

		std::swap(style.WindowRounding, windowRounding);
		std::swap(style.FrameRounding, frameRounding);
	}

}