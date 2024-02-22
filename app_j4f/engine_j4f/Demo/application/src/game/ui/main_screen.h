#pragma once

#include "../graphics/ui_manager.h"

namespace game {

	class MainScreen : public UIModule {
		void draw(const float delta) override;
	};

}