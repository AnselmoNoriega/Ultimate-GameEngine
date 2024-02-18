#include "nrpch.h"
#include "Application.h"

#include "Events/ApplicationEvent.h"
#include "NotRed/Log.h"

using namespace NR;

Application::Application()
{
}

Application::~Application()
{
}

void Application::Run()
{
	WindowResizeEvent e(1280, 720);
	NR_TRACE(e);

	while (true);
}
