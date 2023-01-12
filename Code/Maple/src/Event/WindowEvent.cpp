//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "WindowEvent.h"

namespace maple
{
	WindowResizeEvent::WindowResizeEvent(int32_t initWidth, int32_t initHeight) :
	    width(initWidth),
	    height(initHeight)
	{
	}

};        // namespace maple