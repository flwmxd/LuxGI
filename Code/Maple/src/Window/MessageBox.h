//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"

namespace maple::box
{
	/*!
	* Options for styles to apply to a message box
	*/
	enum class Style
	{
		Info,
		Warning,
		Error,
		Question
	};

	/*!
	* Options for buttons to provide on a message box
	*/
	enum class Buttons
	{
		OK,
		OKCancel,
		YesNo,
		Quit
	};

	/*!
 * Possible responses from a message box. 'None' signifies that no option was chosen, and 'Error' signifies that an
 * error was encountered while creating the message box.
 */
	enum class Selection
	{
		OK,
		Cancel,
		Yes,
		No,
		Quit,
		None,
		Error
	};

	/*!
 * The default style to apply to a message box
 */
	const Style kDefaultStyle = Style::Info;

	/*!
 * The default buttons to provide on a message box
 */
	const Buttons kDefaultButtons = Buttons::OK;

	/*!
 * Blocking call to create a modal message box with the given message, title, style, and buttons
 */
	MAPLE_EXPORT auto show(const char *message, const char *title, Style style, Buttons buttons) -> Selection;

	/*!
 * Convenience function to call show() with the default buttons
 */
	inline auto show(const char *message, const char *title, Style style)
	{
		return show(message, title, style, kDefaultButtons);
	}

	/*!
 * Convenience function to call show() with the default style
 */
	inline auto show(const char *message, const char *title, Buttons buttons)
	{
		return show(message, title, kDefaultStyle, buttons);
	}

	/*!
 * Convenience function to call show() with the default style and buttons
 */
	inline auto show(const char *message, const char *title)
	{
		return show(message, title, kDefaultStyle, kDefaultButtons);
	}
}        // namespace maple::box
