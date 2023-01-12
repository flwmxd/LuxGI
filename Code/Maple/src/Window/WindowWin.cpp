//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "WindowWin.h"
#include "Event/Event.h"
#include "Event/WindowEvent.h"
#include <memory>

#ifdef MAPLE_VULKAN
#	define GLFW_INCLUDE_VULKAN
#elif MAPLE_OPENGL
#	include "RHI/OpenGL/GL.h"
#endif        // MAPLE_VULKAN

#include "GLFW/glfw3.h"

#ifdef _WIN32
#	define GLFW_EXPOSE_NATIVE_WIN32
#	include <GLFW/glfw3native.h>        // for glfwGetWin32Window
#endif                                   // _WIN32

#include "Application.h"

#define CAST_TO_WIN(x) static_cast<WindowWin *>(glfwGetWindowUserPointer(x))

namespace maple
{
	WindowWin::WindowWin(const WindowInitData &initData) :
	    data(initData)
	{
	}

	auto WindowWin::onUpdate() -> void
	{
		glfwPollEvents();
	}

	auto WindowWin::setVSync(bool vsync) -> void
	{
#ifndef MAPLE_VULKAN
		glfwSwapInterval(vsync ? 1 : 0);
#endif
	}

	auto WindowWin::setTitle(const std::string &title) -> void
	{
#ifdef MAPLE_VULKAN
		auto t = title + "(Vulkan)";
#else
		auto t = title + "(OpenGL)";
#endif        // MAPLE_VULKAN
		glfwSetWindowTitle(nativeInterface, t.c_str());
	}

	auto WindowWin::init() -> void
	{
		if (!glfwInit())
			return;

#ifdef MAPLE_VULKAN
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif        // MAPLE_VULKAN

		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		//glfwWindowHint(GLFW_SCALE_TO_MONITOR, 1);
		glfwWindowHint(GLFW_SAMPLES, 4);
#ifdef MAPLE_OPENGL
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#	ifdef PLATFORM_MAC
		glfwWindowHint(GLFW_SAMPLES, 1);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);
		glfwWindowHint(GLFW_COCOA_GRAPHICS_SWITCHING, GL_TRUE);
		glfwWindowHint(GLFW_STENCIL_BITS, 8);
		glfwWindowHint(GLFW_STEREO, GLFW_FALSE);
		glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
#	endif

#endif        // MAPLE_OPENGL

		GLFWmonitor *monitor = glfwGetPrimaryMonitor();
		/*float        xscale, yscale;
		glfwGetMonitorContentScale(monitor, &xscale, &yscale);
		if (xscale > 1 || yscale > 1)
		{
			scale = xscale;
			glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
		}*/

		nativeInterface = glfwCreateWindow(data.width, data.height, data.title.c_str(), nullptr, nullptr);

		if (!nativeInterface)
			return;

		setTitle(data.title);

		glfwMakeContextCurrent(nativeInterface);
#ifdef MAPLE_OPENGL
		if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
		{
			LOGE("Failed to load OpenGL library");
		}
#endif
		glfwSetInputMode(nativeInterface, GLFW_STICKY_KEYS, 1);
		glfwSetWindowUserPointer(nativeInterface, this);
		registerNativeEvent(data);
		setVSync(false);

		int32_t x;
		int32_t y;
		glfwGetWindowPos(nativeInterface,&x,&y);
		pos = {x, y};
	}

	auto WindowWin::swapBuffers() -> void
	{
#ifdef MAPLE_OPENGL
		glfwSwapBuffers(nativeInterface);
#endif        // MAPLE_OPENGL
	}

	auto WindowWin::isClose() const -> bool
	{
		return glfwWindowShouldClose(nativeInterface) == 1;
	}

	auto WindowWin::registerNativeEvent(const WindowInitData &data) -> void
	{
		glfwSetWindowSizeCallback(nativeInterface, [](GLFWwindow *win, int32_t w, int32_t h) {
			Application::getEventDispatcher().postEvent(std::make_unique<WindowResizeEvent>(w, h));
			Application::get()->onWindowResized(w, h);
		});

		glfwSetMouseButtonCallback(nativeInterface, [](GLFWwindow *window, int32_t btnId, int32_t state, int32_t mods) {
			auto w = (WindowWin *) glfwGetWindowUserPointer(window);

			double x;
			double y;
			glfwGetCursorPos(window, &x, &y);

			auto btn = -1;
			switch (btnId)
			{
				case GLFW_MOUSE_BUTTON_LEFT:
					btn = 0;
					break;
				case GLFW_MOUSE_BUTTON_MIDDLE:
					btn = 1;
					break;
				case GLFW_MOUSE_BUTTON_RIGHT:
					btn = 2;
					break;
			}

			if (state == GLFW_PRESS || state == GLFW_REPEAT)
			{
				Application::getEventDispatcher().postEvent(std::make_unique<MouseClickEvent>(btn, x, y));
			}
			if (state == GLFW_RELEASE)
			{
				Application::getEventDispatcher().postEvent(std::make_unique<MouseReleaseEvent>(btn, x, y));
			}
		});

		glfwSetCursorPosCallback(nativeInterface, [](GLFWwindow *window, double x, double y) {
			auto w = (WindowWin *) glfwGetWindowUserPointer(window);
			Application::getEventDispatcher().postEvent(std::make_unique<MouseMoveEvent>(x, y));
		});

		glfwSetScrollCallback(nativeInterface, [](GLFWwindow *win, double xOffset, double yOffset) {
			double x;
			double y;
			glfwGetCursorPos(win, &x, &y);
			Application::getEventDispatcher().postEvent(std::make_unique<MouseScrolledEvent>(xOffset, yOffset, x, y));
		});

		glfwSetCharCallback(nativeInterface, [](GLFWwindow *window, unsigned int keycode) {
			Application::getEventDispatcher().postEvent(std::make_unique<CharInputEvent>(KeyCode::Id(keycode), (char) keycode));
		});

		glfwSetKeyCallback(nativeInterface, [](GLFWwindow *, int32_t key, int32_t scancode, int32_t action, int32_t mods) {
			switch (action)
			{
				case GLFW_PRESS: {
					Application::getEventDispatcher().postEvent(std::make_unique<KeyPressedEvent>(static_cast<KeyCode::Id>(key), 0));
					break;
				}
				case GLFW_RELEASE: {
					Application::getEventDispatcher().postEvent(std::make_unique<KeyReleasedEvent>(static_cast<KeyCode::Id>(key)));
					break;
				}
				case GLFW_REPEAT: {
					Application::getEventDispatcher().postEvent(std::make_unique<KeyPressedEvent>(static_cast<KeyCode::Id>(key), 1));
					break;
				}
			}
		});

		glfwSetWindowPosCallback(nativeInterface, [](GLFWwindow *win, int x, int y) {
			WindowWin *win2 = static_cast<WindowWin *>(glfwGetWindowUserPointer(win));
			win2->pos       = {x, y};
		});
	}

	auto WindowWin::getNativeWindow() -> void *
	{
		return glfwGetWin32Window(nativeInterface);
	}
};        // namespace maple
