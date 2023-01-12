//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "Application.h"
#include "Others/Console.h"

extern maple::Application *createApplication();

auto main() -> int32_t
{
	maple::Console::init();
	maple::Application::app = createApplication();
	auto retCode            = maple::Application::app->start();
	delete maple::Application::app;
	return retCode;
}
