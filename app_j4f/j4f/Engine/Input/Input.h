#pragma once

#include "../Core/EngineModule.h"

#include <vector>
#include <cstdint>
#include <algorithm>

namespace engine {

	enum class InputEventState : uint8_t {
		IES_NONE = 0,
		IES_PRESS = 1,
		IES_RELEASE = 2
	};

	enum class PointerButton : uint8_t {
		PBUTTON_0 = 0,
		PBUTTON_1 = 1,
		PBUTTON_2 = 2,
		PBUTTON_3 = 3,
		PBUTTON_4 = 4,
		PBUTTON_5 = 5,
		PBUTTON_6 = 6,
		PBUTTON_7 = 7,
		PBUTTON_8 = 8,
		PBUTTON_9 = 9, 
		PBUTTON_10 = 10,
		PBUTTON_NONE = 255,
	};

	enum class KeyboardKey : uint8_t {
		K_SPACE = 0,
		K_APOSTROPHE = 2,
		K_COMMA = 3,
		K_MINUS = 4, 
		K_PERIOD = 5,
		K_SLASH = 6,
		K_0 = 7,
		K_1 = 8,
		K_2 = 9,
		K_3 = 10,
		K_4 = 11,
		K_5 = 12,
		K_6 = 13,
		K_7 = 14,
		K_8 = 15,
		K_9 = 16,
		K_SEMICOLON = 17,
		K_EQUAL = 18,
		K_A = 19,
		K_B = 20,
		K_C = 21,
		K_D = 22,
		K_E = 23,
		K_F = 24,
		K_G = 25,
		K_H = 26,
		K_I = 27,
		K_J = 28,
		K_K = 29,
		K_L = 30,
		K_M = 31,
		K_N = 32,
		K_O = 33,
		K_P = 34,
		K_Q = 35,
		K_R = 36,
		K_S = 37,
		K_T = 38,
		K_U = 39,
		K_V = 40,
		K_W = 41,
		K_X = 42,
		K_Y = 43,
		K_Z = 44,
		K_LEFT_BRACKET = 45,
		K_BACKSLASH = 46,
		K_RIGHT_BRACKET = 47,
		K_GRAVE_ACCENT = 48,

		K_ESCAPE = 49,
		K_ENTER = 50,
		K_TAB = 51,
		K_BACKSPACE = 52,
		K_INSERT = 53,
		K_DELETE = 54,
		K_RIGHT = 55,
		K_LEFT = 56,
		K_DOWN = 57,
		K_UP = 58,
		K_PAGE_UP = 59,
		K_PAGE_DOWN = 60,
		K_HOME = 61,
		K_END = 62,
		K_CAPS_LOCK = 63,
		K_SCROLL_LOCK = 64,
		K_NUM_LOCK = 65,
		K_PRINT_SCREEN = 66,
		K_PAUSE = 67,
		K_F1 = 68,
		K_F2 = 69,
		K_F3 = 70,
		K_F4 = 71,
		K_F5 = 72,
		K_F6 = 73,
		K_F7 = 74,
		K_F8 = 75,
		K_F9 = 76,
		K_F10 = 77,
		K_F11 = 78,
		K_F12 = 79,
		K_F13 = 80,
		K_F14 = 81,
		K_F15 = 82,
		K_F16 = 83,
		K_F17 = 84,
		K_F18 = 85,
		K_F19 = 86,
		K_F20 = 87,
		K_F21 = 88,
		K_F22 = 89,
		K_F23 = 90,
		K_F24 = 91,
		K_F25 = 92,
		K_KP_0 = 93,
		K_KP_1 = 94,
		K_KP_2 = 95,
		K_KP_3 = 96,
		K_KP_4 = 97,
		K_KP_5 = 98,
		K_KP_6 = 99,
		K_KP_7 = 100,
		K_KP_8 = 101,
		K_KP_9 = 102,
		K_KP_DECIMAL = 103,
		K_KP_DIVIDE = 104,
		K_KP_MULTIPLY = 105,
		K_KP_SUBTRACT = 106,
		K_KP_ADD = 107,
		K_KP_ENTER = 108,
		K_KP_EQUAL = 109,
		K_LEFT_SHIFT = 110,
		K_LEFT_CONTROL = 111,
		K_LEFT_ALT = 112,
		K_LEFT_SUPER = 113,
		K_RIGHT_SHIFT = 114,
		K_RIGHT_CONTROL = 115,
		K_RIGHT_ALT = 116,
		K_RIGHT_SUPER = 117,
		K_MENU = 118,
		K_UNKNOWN = 255
	};

	struct PointerEvent {
		PointerButton button;
		InputEventState state;
		float x;
		float y;

		PointerEvent(const PointerButton btn, const InputEventState st, const float px, const float py) : button(btn), state(st), x(px), y(py) {}
	};

	struct KeyEvent {
		KeyboardKey key;
		InputEventState state;

		KeyEvent(const KeyboardKey k, const InputEventState st) : key(k), state(st) {}
	};

	class InputObserver {
	public:
		virtual ~InputObserver() = default;
		virtual bool onInputPointerEvent(const PointerEvent& event) = 0;
		virtual bool onInputWheelEvent(const float dx, const float dy) = 0;
		virtual bool onInpuKeyEvent(const KeyEvent& event) = 0;
		virtual bool onInpuCharEvent(const uint16_t code) = 0;

		inline void setPriority(const uint16_t p) { _priority = p; }
		inline uint16_t getPriority() const { return _priority; }
	private:
		uint16_t _priority = 0;
	};

	class Input : public IEngineModule {
	public:
		void addObserver(InputObserver* o) {
			_observers.push_back(o);
			if (_observers.size() > 1) {
				std::sort(_observers.begin(), _observers.end(), [](const InputObserver* o1, InputObserver* o2) { return o1->getPriority() > o2->getPriority(); });
			}
		}
		
		void removeObserver(InputObserver* o) {
			_observers.erase(std::remove(_observers.begin(), _observers.end(), o), _observers.end());
		}

		template <typename PE = PointerEvent&&>
		inline void onPointerEvent(PE&& evt) const {
			for (auto&& o : _observers) {
				if (o->onInputPointerEvent(evt)) {
					break;
				}
			}
		}

		template <typename KE = KeyEvent&&>
		inline void onKeyEvent(KE&& evt) const {
			for (auto&& o : _observers) {
				if (o->onInpuKeyEvent(evt)) {
					break;
				}
			}
		}

		inline void onWheelEvent(const float dx, const float dy) const {
			for (auto&& o : _observers) {
				if (o->onInputWheelEvent(dx, dy)) {
					break;
				}
			}
		}

		inline void onCharEvent(const uint16_t code) const {
			for (auto&& o : _observers) {
				if (o->onInpuCharEvent(code)) {
					break;
				}
			}
		}

	private:
		std::vector<InputObserver*> _observers;
	};
}