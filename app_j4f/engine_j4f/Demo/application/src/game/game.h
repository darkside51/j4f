#pragma once

#include <Engine/Input/Input.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace game{
	class Scene;
}

namespace engine {
	class Game : public InputObserver {
	public:
		Game();
		~Game();

		void onEngineInitComplete();
		void update(const float delta);
		void render(const float delta);
		void resize(const uint16_t w, const uint16_t h);

		bool onInputPointerEvent(const PointerEvent& event) override;
		bool onInputWheelEvent(const float dx, const float dy) override;
		bool onInpuKeyEvent(const KeyEvent& event) override;
		bool onInpuCharEvent(const uint16_t code) override;

	private:
		std::unique_ptr<game::Scene> _scene;
	};
}