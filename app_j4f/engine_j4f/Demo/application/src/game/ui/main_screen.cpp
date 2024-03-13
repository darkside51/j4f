#include "main_screen.h"

#include <Engine/Core/Engine.h>
#include <Engine/Events/Bus.h>
#include <Engine/Utils/StringHelper.h>

#include <imgui.h>
#include <array>

namespace game {

	constexpr float kWindowRound = 20.0f;
	constexpr float kFrameRound = 10.0f;

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

	float MainScreen::drawBottomPanel() {
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
			| ImGuiWindowFlags_NoSavedSettings
			| ImGuiWindowFlags_NoFocusOnAppearing
			| ImGuiWindowFlags_NoNav
			| ImGuiWindowFlags_NoMove;

		const float height = 70.0f;
		const float btnSize = height - 10.0f;
		const uint8_t btnCount = 6u;
		const float btnSpace = 5.0f;
		const float width = (btnSize + btnSpace) * btnCount - btnSpace + 2 * (kWindowRound - kFrameRound);
		const float hspace = 10.0f;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		const ImVec2 work_size = viewport->WorkSize;

		ImVec2 window_pos;
		window_pos.x = (work_size.x - width) * 0.5f;
		window_pos.y = work_size.y - (height + hspace);
		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);

		ImGuiStyle& style = ImGui::GetStyle();
		auto windowRounding = kWindowRound;
		auto frameRounding = kFrameRound;
		std::swap(style.WindowRounding, windowRounding);
		std::swap(style.FrameRounding, frameRounding);

		if (ImGui::Begin("##skills_panel", nullptr, window_flags)) {

			for (uint8_t i = 0u; i < btnCount; ++i) {
				ImGui::SetCursorPos({ (kWindowRound - kFrameRound) + (btnSize + btnSpace) * i, 5.0f });
				if (ImGui::Button(engine::fmt_string("#act_{}", i), { btnSize, btnSize })) {
					engine::Engine::getInstance().getModule<engine::Bus>().sendEvent(i);
				}
			}

			ImGui::End();
		}

		std::swap(style.WindowRounding, windowRounding);
		std::swap(style.FrameRounding, frameRounding);

		return width;
	}

	void MainScreen::drawMapPanel(const float x, float delta) {
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
			| ImGuiWindowFlags_NoSavedSettings
			| ImGuiWindowFlags_NoFocusOnAppearing
			| ImGuiWindowFlags_NoNav
			| ImGuiWindowFlags_NoMove;
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		const ImVec2 work_size = viewport->WorkSize;

		constexpr float kLocatorSpeed = 90.0f;
		const float width = 100.0f;
		const float height = _showMap ? width : 10.0f;
		const float wspace = 10.0f;
		const float hspace = 10.0f;

		ImVec2 window_pos;
		window_pos.x = std::max(x - wspace - width, 0.0f);
		window_pos.y = work_size.y - (height + hspace);
		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);

		ImGuiStyleColorChanger bg{ ImGuiCol_WindowBg, IM_COL32(100, 100, 100, 200) };

		if (ImGui::Begin("##map_panel", nullptr, window_flags)) {
			ImGui::SetCursorPos({0.0f, 0.0f});
			if (ImGui::Button("##minimap", { width, 10.0f })) {
				_showMap = !_showMap;
			}
			if (_showMap) {
				auto&& drawList = ImGui::GetWindowDrawList();
				drawList->AddRectFilled({ window_pos.x + width * 0.5f - 0.5f, window_pos.y }, { window_pos.x + width * 0.5f + 0.5f, window_pos.y + height }, 0x33'ff'ff'ffu);
				drawList->AddRectFilled({ window_pos.x, window_pos.y + height * 0.5f - 0.5f }, { window_pos.x + width, window_pos.y + height * 0.5f + 0.5f }, 0x33'ff'ff'ffu);
				drawList->AddCircleFilled({ window_pos.x + width * 0.5f, window_pos.y + height * 0.5f }, 3.0f, 0xff'77'77'ffu, 16);

				_locatorSearchRadius += delta * kLocatorSpeed;
				drawList->AddCircle({ window_pos.x + width * 0.5f, window_pos.y + height * 0.5f }, _locatorSearchRadius, 0xff'77'77'77u, 32);
				if (_locatorSearchRadius >= width * 0.5f) {
					_locatorSearchRadius = 0.0f;
				}
			}
			ImGui::End();
		}
	}

	void MainScreen::draw(const float delta) {
		//ImGui::ShowDemoWindow();

		const auto bgColor = IM_COL32(200, 200, 0, 100);
		const std::array<ImGuiStyleColorChanger, 14u> changedColors = {
				ImGuiStyleColorChanger{ImGuiCol_Text, IM_COL32(200, 200, 200, 200)},
				{ImGuiCol_Button, IM_COL32(20, 20, 20, 100)},
				{ImGuiCol_ButtonActive, IM_COL32(0, 0, 0, 50)},
				{ImGuiCol_ButtonHovered, IM_COL32(0, 0, 0, 100)},
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

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		const ImVec2 work_size = viewport->WorkSize;

		auto const w = drawBottomPanel();
		drawMapPanel((work_size.x - w) * 0.5f, delta);
	}

}