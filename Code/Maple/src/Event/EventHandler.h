//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cstdint>
#include <functional>
#include <queue>
#include <set>

#include "Event.h"

namespace maple
{
	class EventDispatcher;

	class EventHandler final
	{
	  public:
		friend EventDispatcher;
		static constexpr int32_t PRIORITY_MAX = 0x1000;

		explicit EventHandler(int32_t initPriority = 0) :
		    priority(initPriority)
		{
		}

		~EventHandler();

		std::function<bool(MouseMoveEvent *)>     mouseMoveHandler;
		std::function<bool(MouseClickEvent *)>    mouseClickHandler;
		std::function<bool(MouseReleaseEvent *)>  mouseRelaseHandler;
		std::function<bool(MouseScrolledEvent *)> mouseScrollHandler;
		std::function<bool(KeyPressedEvent *)>    keyPressedHandler;
		std::function<bool(KeyReleasedEvent *)>   keyReleasedHandler;
		std::function<bool(CharInputEvent *)>     charInputHandler;
		std::function<bool(DeferredTypeEvent *)>  deferredTypeHandler;

		auto remove() -> void;

	  private:
		int32_t          priority;
		EventDispatcher *eventDispatcher = nullptr;
	};
};        // namespace maple
