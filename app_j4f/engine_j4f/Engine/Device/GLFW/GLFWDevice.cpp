#include "GLFWDevice.h"
#include "../../Graphics/RenderSurfaceInitializer.h"

#include "../../Core/Engine.h"
#include "../../Input/Input.h"
//#include "../../Log/Log.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>
#include <thread>

#ifdef VK_USE_PLATFORM_XCB_KHR
#include <dlfcn.h>
#endif

namespace engine {

	class GlfwVkSurfaceInitializer : public IRenderSurfaceInitializer {
	public:
		explicit GlfwVkSurfaceInitializer(GLFWwindow* window) : _window(window) { }

		bool initRenderSurface(void* renderInstane, void* renderSurace) const override { 
#if defined VK_USE_PLATFORM_WIN32_KHR // windows variant
			VkWin32SurfaceCreateInfoKHR vk_surfaceInfo;
			vk_surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			vk_surfaceInfo.pNext = nullptr;
			vk_surfaceInfo.flags = 0;
			vk_surfaceInfo.hinstance = GetModuleHandle(nullptr);
			vk_surfaceInfo.hwnd = glfwGetWin32Window(_window);

			return vkCreateWin32SurfaceKHR(*(static_cast<VkInstance*>(renderInstane)), &vk_surfaceInfo,
                                           nullptr, static_cast<VkSurfaceKHR*>(renderSurace)) == VK_SUCCESS;
#elif defined VK_USE_PLATFORM_WAYLAND_KHR
            // https://github.com/googlesamples/vulkan-basic-samples/blob/master/Sample-Programs/Hologram/ShellWayland.cpp
			return false; // todo!
#elif defined VK_USE_PLATFORM_XCB_KHR
            using xcbConnectionGetter = xcb_connection_t* (*)(Display*);
            auto x11handle = dlopen("libX11-xcb.so.1", RTLD_LAZY | RTLD_LOCAL);
            auto getXCBConnection = reinterpret_cast<xcbConnectionGetter>(dlsym(x11handle, "XGetXCBConnection"));
            xcb_connection_t* connection = getXCBConnection(glfwGetX11Display());
            dlclose(x11handle);

            VkXcbSurfaceCreateInfoKHR vk_surfaceInfo;
            vk_surfaceInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
            vk_surfaceInfo.pNext = nullptr;
            vk_surfaceInfo.flags = 0;
            vk_surfaceInfo.connection = connection;
            vk_surfaceInfo.window = glfwGetX11Window(_window);

            return vkCreateXcbSurfaceKHR(*(static_cast<VkInstance*>(renderInstane)), &vk_surfaceInfo,
                                         nullptr, static_cast<VkSurfaceKHR*>(renderSurace));
#elif defined VK_USE_PLATFORM_XLIB_KHR
            VkXlibSurfaceCreateInfoKHR vk_surfaceInfo;
            vk_surfaceInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
            vk_surfaceInfo.pNext = nullptr;
            vk_surfaceInfo.flags = 0;
            vk_surfaceInfo.dpy = glfwGetX11Display();
            vk_surfaceInfo.window = glfwGetX11Window(_window);

			return vkCreateXlibSurfaceKHR(*(static_cast<VkInstance*>(renderInstane)), &vk_surfaceInfo,
                                          nullptr, static_cast<VkSurfaceKHR*>(renderSurace));
#endif
		}

        [[nodiscard]] uint32_t getDesiredImageCount() const override { return 3; }

	private:
		GLFWwindow* _window = nullptr;
	};

	void glfwOnWindowResize(GLFWwindow* /*window*/, int w, int h) {
//        LOG_TAG_LEVEL(LogLevel::L_MESSAGE, DEVICE, "resize %d x %d", w, h);
		const auto uw = static_cast<uint16_t>(w);
		const auto uh = static_cast<uint16_t>(h);
		Engine::getInstance().getModule<GLFWDevice>()->setSize(uw, uh);
	}

//	void glfwOnFrameBufferResize(GLFWwindow*, int w, int h) {
////        LOG_TAG_LEVEL(LogLevel::L_MESSAGE, DEVICE, "resize %d x %d", w, h);
//		const auto uw = static_cast<uint16_t>(w);
//		const auto uh = static_cast<uint16_t>(h);
//		Engine::getInstance().getModule<GLFWDevice>()->setSize(uw, uh);
//	}

	void glfwOnWindowIconify(GLFWwindow*, int /*iconified*/) {
		//iconified ? printf("iconify window\n") : printf("restore window\n");
	}

	void glfwOnMouseMove(GLFWwindow* window, double x, double y) {
		int w, h;
		glfwGetWindowSize(window, &w, &h);
		Engine::getInstance().getModule<Input>()->onPointerEvent(PointerEvent(PointerButton::PBUTTON_NONE,
                                                                              InputEventState::IES_NONE,
                                                                              static_cast<float>(x), static_cast<float>(h) - static_cast<float>(y)));
	}

	void glfwOnMouseButton(GLFWwindow* window, int button, int action, int /*mods*/) {
		double x, y;
		int w, h;
		glfwGetCursorPos(window, &x, &y);
		glfwGetWindowSize(window, &w, &h);
		Engine::getInstance().getModule<Input>()->onPointerEvent(PointerEvent(static_cast<PointerButton>(button),
                                                                              (action == 0 ? InputEventState::IES_RELEASE : InputEventState::IES_PRESS),
                                                                              static_cast<float>(x), static_cast<float>(h) - static_cast<float>(y)));
	}

	void glfwOnScroll(GLFWwindow* /*window*/, double xoffset, double yoffset) { // mouse wheel
		Engine::getInstance().getModule<Input>()->onWheelEvent(static_cast<float>(xoffset), static_cast<float>(yoffset));
	}

	void glfwOnKeyboard(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int /*mods*/) {

		KeyboardKey k;
		uint8_t specialMask = 0;

		if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
			k = static_cast<KeyboardKey>((key - GLFW_KEY_0) + static_cast<int>(KeyboardKey::K_0));
		} else if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
			k = static_cast<KeyboardKey>((key - GLFW_KEY_A) + static_cast<int>(KeyboardKey::K_A));
		} else if (key >= GLFW_KEY_ESCAPE && key <= GLFW_KEY_F25) {
			k = static_cast<KeyboardKey>((key - GLFW_KEY_ESCAPE) + static_cast<int>(KeyboardKey::K_ESCAPE));
		} else if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_MENU) {
			k = static_cast<KeyboardKey>((key - GLFW_KEY_KP_0) + static_cast<int>(KeyboardKey::K_KP_0));

			switch (k) {
				case KeyboardKey::K_LEFT_ALT:
				case KeyboardKey::K_RIGHT_ALT:
					specialMask |= (1 << Input::ALT_PRESSED_BIT);
					break;
				case KeyboardKey::K_LEFT_CONTROL:
				case KeyboardKey::K_RIGHT_CONTROL:
					specialMask |= (1 << Input::CTRL_PRESSED_BIT);
					break;
				case KeyboardKey::K_LEFT_SHIFT:
				case KeyboardKey::K_RIGHT_SHIFT:
					specialMask |= (1 << Input::SHIFT_PRESSED_BIT);
					break;
				case KeyboardKey::K_LEFT_SUPER:
				case KeyboardKey::K_RIGHT_SUPER:
					specialMask |= (1 << Input::SUPER_PRESSED_BIT);
					break;
				default:
					break;
			}
		} else {
			switch (key) {
				case GLFW_KEY_SPACE:
					k = KeyboardKey::K_SPACE;
					break;
				case GLFW_KEY_APOSTROPHE:
					k = KeyboardKey::K_APOSTROPHE;
					break;
				case GLFW_KEY_COMMA:
					k = KeyboardKey::K_COMMA;
					break;
				case GLFW_KEY_MINUS:
					k = KeyboardKey::K_MINUS;
					break;
				case GLFW_KEY_PERIOD:
					k = KeyboardKey::K_PERIOD;
					break;
				case GLFW_KEY_SLASH:
					k = KeyboardKey::K_SLASH;
					break;
				case GLFW_KEY_SEMICOLON:
					k = KeyboardKey::K_SEMICOLON;
					break;
				case GLFW_KEY_EQUAL:
					k = KeyboardKey::K_EQUAL;
					break;
				case GLFW_KEY_LEFT_BRACKET:
					k = KeyboardKey::K_LEFT_BRACKET;
					break;
				case GLFW_KEY_BACKSLASH:
					k = KeyboardKey::K_BACKSLASH;
					break;
				case GLFW_KEY_RIGHT_BRACKET:
					k = KeyboardKey::K_SPACE;
					break;
				case GLFW_KEY_GRAVE_ACCENT:
					k = KeyboardKey::K_GRAVE_ACCENT;
					break;
				default:
                    k = KeyboardKey::K_UNKNOWN;
					action = 255; // for no call onKeyEvent
					break;
			}
		}

		switch (action) {
			case GLFW_PRESS:
				if (specialMask) {
					Engine::getInstance().getModule<Input>()->changeSpecialMask(specialMask, InputEventState::IES_PRESS);
				}
				Engine::getInstance().getModule<Input>()->onKeyEvent(KeyEvent(k, InputEventState::IES_PRESS));
				break;
			case GLFW_RELEASE:
				if (specialMask) {
					Engine::getInstance().getModule<Input>()->changeSpecialMask(specialMask, InputEventState::IES_RELEASE);
				}
				Engine::getInstance().getModule<Input>()->onKeyEvent(KeyEvent(k, InputEventState::IES_RELEASE));
				break;
			default:
				break;
		}
	}

	void glfwOnChar(GLFWwindow* /*window*/, unsigned int codepoint) {
		Engine::getInstance().getModule<Input>()->onCharEvent(codepoint);
	}

	GLFWDevice::GLFWDevice() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		_width = 1024;
		_height = 768;

		//GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		//const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		//_width = mode->width;
		//_height = mode->height;

		_window = glfwCreateWindow(_width, _height, "j4f (vulkan)", nullptr, nullptr);
		//_window = glfwCreateWindow(_width, _height, "j4f (vulkan)", monitor, nullptr);

		//glfwSetFramebufferSizeCallback(_window, &glfwOnFrameBufferResize);
		glfwSetWindowSizeCallback(_window, &glfwOnWindowResize);
		glfwSetWindowIconifyCallback(_window, &glfwOnWindowIconify);

		// input callbacks
		glfwSetCursorPosCallback(_window, &glfwOnMouseMove);
		glfwSetMouseButtonCallback(_window, &glfwOnMouseButton);
		glfwSetScrollCallback(_window, &glfwOnScroll);
		glfwSetKeyCallback(_window, &glfwOnKeyboard);
		glfwSetCharCallback(_window, &glfwOnChar);

		_surfaceInitializer = new GlfwVkSurfaceInitializer(_window);
	}

	GLFWDevice::~GLFWDevice() {
        if (_window) {
            glfwDestroyWindow(_window);
            _window = nullptr;
        }

        if (_surfaceInitializer) {
            delete _surfaceInitializer;
            _surfaceInitializer = nullptr;
        }

		glfwTerminate();
	}

	void GLFWDevice::setFullscreen(const bool fullscreen) {
        static uint16_t lastWindowWidth = 0;
        static uint16_t lastWindowHeight = 0;
        static int lastWindowPosX = 0;
        static int lastWindowPosY = 0;

		if (_fullscreen == fullscreen) return;

		if (_fullscreen = fullscreen) {
            lastWindowWidth = _width;
            lastWindowHeight = _height;
            glfwGetWindowPos(_window, &lastWindowPosX, &lastWindowPosY);
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			glfwSetWindowMonitor(_window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
		} else {
			glfwSetWindowMonitor(_window, nullptr, 0, 0, lastWindowWidth, lastWindowHeight, GLFW_DONT_CARE);
			glfwSetWindowPos(_window, lastWindowPosX, lastWindowPosY);
		}
	}

	bool GLFWDevice::isFullscreen() const {
		return _fullscreen;
	}

	void GLFWDevice::setSize(const uint16_t w, const uint16_t h) {
		_width = w;
		_height = h;
		Engine::getInstance().resize(w, h);
	}

	const IRenderSurfaceInitializer* GLFWDevice::getSurfaceInitializer() const {
		return _surfaceInitializer;
	}

	void GLFWDevice::start() {
		//constexpr float kDeviceSleepMilliseconds = 33.333; // ~ 30fps
        constexpr float kDeviceSleepMilliseconds = 16.667f; // ~ 60fps

		while (!glfwWindowShouldClose(_window)) {
			//if (_width != 0 && _height != 0) {
				//Engine::getInstance().nextFrame();
			//}
            //glfwWaitEvents();

            glfwPollEvents();
            std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(kDeviceSleepMilliseconds));
		}

		stop();
	}

	void GLFWDevice::leaveMainLoop() {
		glfwSetWindowShouldClose(_window, 1);
	}

	void GLFWDevice::stop() {
		Engine::getInstance().deviceDestroyed();
		Engine::getInstance().destroy();
	}
}