//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Devices/KeyCodes.h"
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <queue>
#include <string>

//some events

#define GENERATE_EVENT_CLASS_TYPE(type)                \
	static EventType getEventType()                    \
	{                                                  \
		return EventType::type;                        \
	}                                                  \
	virtual auto getType() const->EventType override   \
	{                                                  \
		return getEventType();                         \
	}                                                  \
	virtual auto getName() const->const char *override \
	{                                                  \
		return #type;                                  \
	}

namespace maple
{
	class Scene;
	enum class EventType
	{
		None,
		WindowClose,
		WindowResize,
		WindowFocus,
		WindowLostFocus,
		Tick,
		Update,
		Render,
		KeyPressed,
		KeyReleased,
		CharInput,
		MouseClicked,
		MouseReleased,
		MouseMove,
		MouseScrolled,
		DeferredType
	};

	class Event
	{
	  public:
		virtual auto getType() const -> EventType    = 0;
		virtual auto getName() const -> const char * = 0;
	};

	class MouseMoveEvent : public Event
	{
	  public:
		MouseMoveEvent(float x, float y) :
		    position(x, y)
		{
		}
		glm::vec2 position;
		GENERATE_EVENT_CLASS_TYPE(MouseMove);
	};

	class MouseClickEvent : public Event
	{
	  public:
		MouseClickEvent(int32_t button, float x, float y) :
		    buttonId(button),
		    position(x, y)
		{
		}
		int32_t   buttonId = -1;
		glm::vec2 position;
		GENERATE_EVENT_CLASS_TYPE(MouseClicked);
	};

	class MouseReleaseEvent : public Event
	{
	  public:
		MouseReleaseEvent(int32_t button, float x, float y) :
		    buttonId(button),
		    position(x, y)
		{
		}
		int32_t   buttonId = -1;
		glm::vec2 position;
		GENERATE_EVENT_CLASS_TYPE(MouseReleased);
	};

	class MouseScrolledEvent : public Event
	{
	  public:
		MouseScrolledEvent(float xOffset, float yOffset, float x, float y) :
		    xOffset(xOffset),
		    yOffset(yOffset),
		    position(x, y)
		{}

		inline auto getXOffset() const
		{
			return xOffset;
		}
		inline auto getYOffset() const
		{
			return yOffset;
		}

		GENERATE_EVENT_CLASS_TYPE(MouseScrolled);

	  private:
		float     xOffset;
		float     yOffset;
		glm::vec2 position;
	};

	class KeyEvent : public Event
	{
	  public:
		inline auto getKeyCode() const
		{
			return keyCode;
		}

	  protected:
		KeyEvent(KeyCode::Id keycode) :
		    keyCode(keycode)
		{}

		KeyCode::Id keyCode;
	};

	class KeyPressedEvent : public KeyEvent
	{
	  public:
		KeyPressedEvent(KeyCode::Id keycode, int repeatCount) :
		    KeyEvent(keycode),
		    repeatCount(repeatCount)
		{}

		inline auto getRepeatCount() const
		{
			return repeatCount;
		}

		GENERATE_EVENT_CLASS_TYPE(KeyPressed);

	  private:
		int32_t repeatCount;
	};

	class KeyReleasedEvent : public KeyEvent
	{
	  public:
		KeyReleasedEvent(KeyCode::Id keycode) :
		    KeyEvent(keycode)
		{}

		GENERATE_EVENT_CLASS_TYPE(KeyReleased);
	};

	class CharInputEvent : public KeyEvent
	{
	  public:
		CharInputEvent(KeyCode::Id keycode, char ch) :
		    KeyEvent(keycode),
		    character(ch)
		{}

		GENERATE_EVENT_CLASS_TYPE(CharInput);

	  public:
		inline auto &getCharacter() const
		{
			return character;
		}
		inline auto setCharacter(char character)
		{
			this->character = character;
		}

	  private:
		char character;
	};

	class DeferredTypeEvent : public Event
	{
	  public:
		GENERATE_EVENT_CLASS_TYPE(DeferredType);

		inline auto &getDeferredType() const
		{
			return deferredType;
		}
		inline auto setDeferredType(int32_t type)
		{
			this->deferredType = type;
		}

	  private:
		int32_t deferredType;

	  public:
	};

};        // namespace maple