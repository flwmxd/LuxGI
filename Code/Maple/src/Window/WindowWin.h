//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "NativeWindow.h"
#include <memory>

#ifdef PLATFORM_DESKTOP

struct GLFWwindow;

namespace maple
{
	class WindowWin : public NativeWindow
	{
	  public:
		WindowWin(const WindowInitData &data);
		virtual ~WindowWin(){};

		auto onUpdate() -> void override;
		auto setVSync(bool sync) -> void override;
		auto getNativeWindow() -> void * override;
		auto setTitle(const std::string &title) -> void override;
		auto init() -> void override;
		auto swapBuffers() -> void override;
		auto isClose() const -> bool;

		inline auto isVSync() const -> bool override
		{
			return data.vsync;
		}

		inline auto getWidth() const -> uint32_t override
		{
			return data.width;
		};

		inline auto getHeight() const -> uint32_t override
		{
			return data.height;
		};

		inline auto getNativeInterface() -> void * override
		{
			return nativeInterface;
		}

	  private:
		auto registerNativeEvent(const WindowInitData &data) -> void;

		WindowInitData data;
		GLFWwindow *   nativeInterface = nullptr;
	};

};            // namespace maple
#endif        // PLATFORM_DESKTOP
