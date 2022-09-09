#include "GLFWDevice.h"
#include "../../Graphics/RenderSurfaceInitialisez.h"

#include "../../Core/Engine.h"
#include "../../Input/Input.h"
#include "../../Utils/Statistic.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>

namespace engine {

	class GlfwStatObserver : public IStatisticObserver {
	public:
		GlfwStatObserver(GLFWwindow* window) : _window(window) {
			if (auto&& stat = Engine::getInstance().getModule<Statistic>()) {
				stat->addObserver(this);
			}
		}

		~GlfwStatObserver() {
			if (auto&& stat = Engine::getInstance().getModule<Statistic>()) {
				stat->removeObserver(this);
			}
		}

		void statisticUpdate(const Statistic* s) override {
			static char buffer[256];
			snprintf(buffer, 255, "j4f (vulkan) fps: %d, cpuFrameTime : %f, dc: %d", s->fps(), s->cpuFrameTime(), s->drawCalls());

			glfwSetWindowTitle(_window, buffer);
		}
	private:
		GLFWwindow* _window = nullptr;
	};

	class GlfwVkSurfaceInitialiser : public IRenderSurfaceInitialiser {
	public:
		GlfwVkSurfaceInitialiser(GLFWwindow* window) : _window(window) { }

		bool initRenderSurface(void* renderInstane, void* renderSurace) const override { 
#ifdef j4f_PLATFORM_WINDOWS // windows variant
			VkWin32SurfaceCreateInfoKHR vk_surfaceInfo;
			vk_surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			vk_surfaceInfo.pNext = nullptr;
			vk_surfaceInfo.flags = 0;
			vk_surfaceInfo.hinstance = GetModuleHandle(nullptr);
			vk_surfaceInfo.hwnd = glfwGetWin32Window(_window);

			return vkCreateWin32SurfaceKHR(*(static_cast<VkInstance*>(renderInstane)), &vk_surfaceInfo, nullptr, static_cast<VkSurfaceKHR*>(renderSurace)) == VK_SUCCESS;
#else
			return false; // todo!
#endif
		}

		uint32_t getDesiredImageCount() const override { return 3; }

	private:
		GLFWwindow* _window = nullptr;
	};

	void glfwOnWindowResize(GLFWwindow*, int w, int h) {
		const uint16_t uw = static_cast<uint16_t>(w);
		const uint16_t uh = static_cast<uint16_t>(h);
		Engine::getInstance().getModule<GLFWDevice>()->setSize(uw, uh);
	}

	void glfwOnWindowIconify(GLFWwindow*, int iconified) {
		//iconified ? printf("iconify window\n") : printf("restore window\n");
	}

	void glfwOnMouseMove(GLFWwindow* window, double x, double y) {
		int w, h;
		glfwGetWindowSize(window, &w, &h);
		Engine::getInstance().getModule<Input>()->onPointerEvent(PointerEvent(PointerButton::PBUTTON_NONE, InputEventState::IES_NONE, x, h - y));
	}

	void glfwOnMouseButton(GLFWwindow* window, int button, int action, int mods) {
		double x, y;
		int w, h;
		glfwGetCursorPos(window, &x, &y);
		glfwGetWindowSize(window, &w, &h);
		Engine::getInstance().getModule<Input>()->onPointerEvent(PointerEvent(static_cast<PointerButton>(button), (action == 0 ? InputEventState::IES_RELEASE : InputEventState::IES_PRESS), x, h - y));
	}

	void glfwOnScroll(GLFWwindow* window, double xoffset, double yoffset) { // mouse wheel
		Engine::getInstance().getModule<Input>()->onWheelEvent(xoffset, yoffset);
	}

	void glfwOnKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods) {

		KeyBoardKey k = KeyBoardKey::K_UNKNOWN;

		if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
			k = static_cast<KeyBoardKey>((key - GLFW_KEY_0) + static_cast<int>(KeyBoardKey::K_0));
		} else if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
			k = static_cast<KeyBoardKey>((key - GLFW_KEY_A) + static_cast<int>(KeyBoardKey::K_A));
		} else if (key >= GLFW_KEY_ESCAPE && key <= GLFW_KEY_F25) {
			k = static_cast<KeyBoardKey>((key - GLFW_KEY_ESCAPE) + static_cast<int>(KeyBoardKey::K_ESCAPE));
		} else if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_MENU) {
			k = static_cast<KeyBoardKey>((key - GLFW_KEY_KP_0) + static_cast<int>(KeyBoardKey::K_KP_0));
		} else {

		switch (key) {
			case GLFW_KEY_SPACE:
				k = KeyBoardKey::K_SPACE;
				break;
			case GLFW_KEY_APOSTROPHE:
				k = KeyBoardKey::K_APOSTROPHE;
				break;
			case GLFW_KEY_COMMA:
				k = KeyBoardKey::K_COMMA;
				break;
			case GLFW_KEY_MINUS:
				k = KeyBoardKey::K_MINUS;
				break;
			case GLFW_KEY_PERIOD:
				k = KeyBoardKey::K_PERIOD;
				break;
			case GLFW_KEY_SLASH:
				k = KeyBoardKey::K_SLASH;
				break;
			case GLFW_KEY_SEMICOLON:
				k = KeyBoardKey::K_SEMICOLON;
				break;
			case GLFW_KEY_EQUAL:
				k = KeyBoardKey::K_EQUAL;
				break;
			case GLFW_KEY_LEFT_BRACKET:
				k = KeyBoardKey::K_LEFT_BRACKET;
				break;
			case GLFW_KEY_BACKSLASH:
				k = KeyBoardKey::K_BACKSLASH;
				break;
			case GLFW_KEY_RIGHT_BRACKET:
				k = KeyBoardKey::K_SPACE;
				break;
			case GLFW_KEY_GRAVE_ACCENT:
				k = KeyBoardKey::K_GRAVE_ACCENT;
				break;
			default:
				action = 255; // for no call onKeyEvent
				break;
			}
		}

		switch (action) {
			case GLFW_PRESS:
				Engine::getInstance().getModule<Input>()->onKeyEvent(KeyEvent(k, InputEventState::IES_PRESS));
				break;
			case GLFW_RELEASE:
				Engine::getInstance().getModule<Input>()->onKeyEvent(KeyEvent(k, InputEventState::IES_RELEASE));
				break;
			default:
				break;
		}
	}

	void glfwOnChar(GLFWwindow* window, unsigned int codepoint) {
	}

	static GlfwStatObserver* statObserver; // todo remove stat observer from this code

	GLFWDevice::GLFWDevice() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		_width = 1024;
		_height = 768;

		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		//_width = mode->width;
		//_height = mode->height;

		_window = glfwCreateWindow(_width, _height, "j4f (vulkan)", nullptr, nullptr);
		//_window = glfwCreateWindow(_width, _height, "j4f (vulkan)", monitor, nullptr);

		glfwSetWindowSizeCallback(_window, &glfwOnWindowResize);
		glfwSetWindowIconifyCallback(_window, &glfwOnWindowIconify);

		// input callbacks
		glfwSetCursorPosCallback(_window, &glfwOnMouseMove);
		glfwSetMouseButtonCallback(_window, &glfwOnMouseButton);
		glfwSetScrollCallback(_window, &glfwOnScroll);
		glfwSetKeyCallback(_window, &glfwOnKeyboard);
		glfwSetCharCallback(_window, &glfwOnChar);

		_surfaceInitialiser = new GlfwVkSurfaceInitialiser(_window);

		statObserver = new GlfwStatObserver(_window); // todo remove stat observer from this code
	}

	GLFWDevice::~GLFWDevice() {
		glfwTerminate();
		delete _surfaceInitialiser;
	}

	void GLFWDevice::swicthFullscreen(const bool fullscreen) {
		if (fullscreen) {
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			_width = mode->width;
			_height = mode->height;
			glfwSetWindowMonitor(_window, glfwGetPrimaryMonitor(), 0, 0, _width, _height, GLFW_DONT_CARE);
		} else {
			glfwSetWindowMonitor(_window, nullptr, 0, 0, _width, _height, GLFW_DONT_CARE);
		}
	}

	void GLFWDevice::setSize(const uint16_t w, const uint16_t h) {
		_width = w;
		_height = h;
		Engine::getInstance().resize(w, h);
	}

	const IRenderSurfaceInitialiser* GLFWDevice::getSurfaceInitialiser() const {
		return _surfaceInitialiser;
	}

	void GLFWDevice::start() {
		while (!glfwWindowShouldClose(_window)) {
			if (_width != 0 && _height != 0) {
				Engine::getInstance().nextFrame();
			}

			glfwPollEvents();
		}

		stop();
	}

	void GLFWDevice::leaveMainLoop() {
		glfwSetWindowShouldClose(_window, 1);
	}

	void GLFWDevice::stop() {
		delete statObserver; // todo remove stat observer from this code
		Engine::getInstance().deviceDestroyed();
		if (_window) {
			glfwDestroyWindow(_window);
		}

		Engine::getInstance().destroy();		
	}
}