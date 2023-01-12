//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Input.h"
#include "Application.h"

namespace maple
{
	std::unique_ptr<maple::Input> Input::input;

	auto Input::create() -> void
	{
		input = std::make_unique<Input>();
	}

	auto Input::reset() -> void
	{
		memset(keyHeld, 0, MAX_KEYS);
		memset(keyPressed, 0, MAX_KEYS);
		memset(mouseClicked, 0, MAX_BUTTONS);
		memset(mouseHeld, 0, MAX_BUTTONS);

		mouseOnScreen = true;
		windowFocus   = true;
		scrollOffset  = 0.0f;
	}

	auto Input::resetPressed() -> void
	{
		memset(keyPressed, 0, MAX_KEYS);
		memset(mouseClicked, 0, MAX_BUTTONS);
		scrollOffset = 0;
	}

	Input::Input()
	{
		handler.mouseScrollHandler = std::bind(&Input::onMouseScrolled, this, std::placeholders::_1);
		handler.mouseClickHandler  = std::bind(&Input::onMousePressed, this, std::placeholders::_1);
		handler.mouseRelaseHandler = std::bind(&Input::onMouseReleased, this, std::placeholders::_1);
		handler.mouseMoveHandler   = std::bind(&Input::onMouseMoved, this, std::placeholders::_1);
		handler.keyPressedHandler  = std::bind(&Input::onKeyPressed, this, std::placeholders::_1);
		handler.keyReleasedHandler = std::bind(&Input::onKeyReleased, this, std::placeholders::_1);
		Application::get()->getEventDispatcher().addEventHandler(&handler);
		reset();
	}

	auto Input::onKeyPressed(KeyPressedEvent *e) -> bool
	{
		setKeyPressed(KeyCode::Id(e->getKeyCode()), e->getRepeatCount() < 1);
		setKeyHeld(KeyCode::Id(e->getKeyCode()), true);
		return false;
	}

	auto Input::onKeyReleased(KeyReleasedEvent *e) -> bool
	{
		setKeyPressed(KeyCode::Id(e->getKeyCode()), false);
		setKeyHeld(KeyCode::Id(e->getKeyCode()), false);
		return false;
	}

	auto Input::onMousePressed(MouseClickEvent *e) -> bool
	{
		setMouseClicked(KeyCode::MouseKey(e->buttonId), true);
		setMouseHeld(KeyCode::MouseKey(e->buttonId), true);
		return false;
	}

	auto Input::onMouseReleased(MouseReleaseEvent *e) -> bool
	{
		setMouseClicked(KeyCode::MouseKey(e->buttonId), false);
		setMouseHeld(KeyCode::MouseKey(e->buttonId), false);
		return false;
	}

	auto Input::onMouseScrolled(MouseScrolledEvent *e) -> bool
	{
		setScrollOffset(e->getYOffset());
		return false;
	}

	auto Input::onMouseMoved(MouseMoveEvent *e) -> bool
	{
		setMousePosition(e->position);
		return false;
	}

};        // namespace maple
