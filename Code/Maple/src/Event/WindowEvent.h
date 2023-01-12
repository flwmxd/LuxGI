//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Event.h"

namespace maple
{
	class WindowResizeEvent : public Event
	{
	  public:
		WindowResizeEvent(int32_t width, int32_t height);
		inline auto getWidth() const
		{
			return width;
		}
		inline auto getHeight() const
		{
			return height;
		}
		GENERATE_EVENT_CLASS_TYPE(WindowResize);

	  private:
		int32_t width, height;
	};
};        // namespace maple