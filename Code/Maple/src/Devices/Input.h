//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "Event/Event.h"
#include "Event/EventHandler.h"
#include "KeyCodes.h"
#include <glm/glm.hpp>
#define MAX_KEYS 1024
#define MAX_BUTTONS 32

namespace maple
{
	class Event;

	class MAPLE_EXPORT Input
	{
	  public:
		virtual ~Input() = default;
		Input();
		static auto &getInput()
		{
			return input;
		}
		static auto create() -> void;

		inline auto isKeyPressed(KeyCode::Id key) const
		{
			return keyPressed[int32_t(key)];
		}
		inline auto isKeyHeld(KeyCode::Id key) const
		{
			return keyHeld[int32_t(key)];
		}
		inline auto isMouseClicked(KeyCode::MouseKey key) const
		{
			return mouseClicked[int32_t(key)];
		}
		inline auto isMouseHeld(KeyCode::MouseKey key) const
		{
			return mouseHeld[int32_t(key)];
		}

		inline auto setKeyPressed(KeyCode::Id key, bool a)
		{
			keyPressed[int32_t(key)] = a;
		}
		inline auto setKeyHeld(KeyCode::Id key, bool a)
		{
			keyHeld[int32_t(key)] = a;
		}
		inline auto setMouseClicked(KeyCode::MouseKey key, bool a)
		{
			mouseClicked[int32_t(key)] = a;
		}
		inline auto setMouseHeld(KeyCode::MouseKey key, bool a)
		{
			mouseHeld[int32_t(key)] = a;
		}

		auto reset() -> void;
		auto resetPressed() -> void;

	  protected:
		static std::unique_ptr<Input> input;

		auto onKeyPressed(KeyPressedEvent *e) -> bool;
		auto onKeyReleased(KeyReleasedEvent *e) -> bool;
		auto onMousePressed(MouseClickEvent *e) -> bool;
		auto onMouseReleased(MouseReleaseEvent *e) -> bool;
		auto onMouseScrolled(MouseScrolledEvent *e) -> bool;
		auto onMouseMoved(MouseMoveEvent *e) -> bool;

		bool keyPressed[MAX_KEYS];
		bool keyHeld[MAX_KEYS];

		bool mouseHeld[MAX_BUTTONS];
		bool mouseClicked[MAX_BUTTONS];

		float scrollOffset = 0.0f;

		bool         mouseOnScreen;
		bool         windowFocus;
		EventHandler handler;
		glm::vec2    mousePosition;

	  public:
		inline auto &getScrollOffset() const
		{
			return scrollOffset;
		}
		inline auto setScrollOffset(float scrollOffset)
		{
			this->scrollOffset = scrollOffset;
		}

		inline auto &isMouseOnScreen() const
		{
			return mouseOnScreen;
		}
		inline auto setMouseOnScreen(bool mouseOnScreen)
		{
			this->mouseOnScreen = mouseOnScreen;
		}

		inline auto &isWindowFocus() const
		{
			return windowFocus;
		}
		inline auto setWindowFocus(bool windowFocus)
		{
			this->windowFocus = windowFocus;
		}

		inline auto &getMousePosition() const
		{
			return mousePosition;
		}
		inline auto setMousePosition(const glm::vec2 &mousePosition)
		{
			this->mousePosition = mousePosition;
		}
	};
};        // namespace maple
