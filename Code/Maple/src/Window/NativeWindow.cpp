//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "NativeWindow.h"
#include "WindowWin.h"

namespace maple
{
	auto getNativeWindow() -> void *
	{
		return nullptr;
	}

	auto NativeWindow::setTitle(const std::string &title) -> void
	{
	}

	auto NativeWindow::create(const WindowInitData &data) -> std::unique_ptr<NativeWindow>
	{
		return std::make_unique<WindowWin>(data);
	}
};        // namespace maple
