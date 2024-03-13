#pragma once

#include "../graphics/ui_manager.h"
#include <cstdint>

namespace game {

	class MainScreen : public UIModule {
		void draw(const float delta) override;
		float drawBottomPanel();
		void drawMapPanel(const float x, const float delta);

	private:
		bool _showMap = false;
		float _locatorSearchRadius = 0.0f;
	};

}