#pragma once

#ifdef NR_PLATFORM_WINDOWS

extern NR::Application* NR::CreateApplication();

int main(int argc, char** argv)
{
	NR::Application* app = NR::CreateApplication();
	NR_CORE_ASSERT(app, "Client Application is null!");
	app->Run();
	delete app;
}
#else
#error NotRed only supports Windows
#endif