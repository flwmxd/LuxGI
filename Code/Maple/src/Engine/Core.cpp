//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Core.h"

#include "DefaultPrinter.hpp"
#include "FancyPrinter.hpp"
#include "Tracer.hpp"
#include "TracerHandler.hpp"
#include "tracerDef.hpp"
#include <iostream>
#include <signal.h>

#include "Window/MessageBox.h"

using namespace std;
using namespace tracer;

namespace maple
{
	auto printStackTrace(const std::string &input) -> void
	{
		static std::unordered_map<size_t, bool> ignore;

	

		Tracer t1;
		t1.trace();
		DefaultPrinter p1;

		auto cfg = p1.getConfig();

		cfg.prefix        = "";        //!< \brief Perefix (prefix [functionName])
		cfg.seper1        = "";        //!< \brief 1st seperator
		cfg.seper2        = "";        //!< \brief 2nd seperator
		cfg.seper3        = "";        //!< \brief 3rd seperator
		cfg.suffix        = "";        //!< \brief suffix
		cfg.colorFrameNum = "";        //!< \brief ANSI escape sequence for the Frame number color
		cfg.colorNotFound = "";        //!< \brief ANSI escape sequence for the "Not Found" color
		cfg.colorAddress  = "";        //!< \brief ANSI escape sequence for the Address color
		cfg.colorFuncName = "";        //!< \brief ANSI escape sequence for the Function Name color
		cfg.colorLineInfo = "";        //!< \brief ANSI escape sequence for the Line information color
		cfg.colorModule   = "";        //!< \brief ANSI escape sequence for the Frame number color
		cfg.colorModule   = "";        //!< \brief ANSI escape sequence for the Frame number color

		p1.setConfig(cfg);

		p1.setTrace(&t1);
		auto str = p1.generateString();
		size_t hash = std::hash<std::string>{}(str);

		if (ignore[hash])
			return;

		auto selection = maple::box::show(str.c_str(), input.c_str(), maple::box::Style::Warning, maple::box::Buttons::YesNo);
		if (selection == maple::box::Selection::Yes)
		{
			//ignore issue.
			ignore[hash] = true;
		}
		else
		{
			__debugbreak();
		}
	}

	auto enableTracer() -> void
	{
		auto *tHandler = TracerHandler::getTracer();        // Get the singelton
		auto  cfg      = tHandler->getConfig();             // Get the current config

		cfg.callback = [](Tracer *tracer, AbstractPrinter *printer, void *userData) {
			auto str = printer->generateString();
			maple::box::show(str.c_str(), "Crash");
		};
		// Edit cfg
		tHandler->setConfig(cfg);                        // Update the config
		auto printer = PrinterContainer::plain();        // Generates a printer

		// Edit printer config
		tHandler->setup(std::move(printer));        // Sets things up. Now the signal handler is setup
	}
}        // namespace maple
