#pragma once

#ifdef NR_PLATFORM_WINDOWS

extern NR::Application* NR::CreateApplication(int argc, char** argv);
bool gApplicationRunning = true;

int main(int argc, char** argv)
{
	while (gApplicationRunning)
	{
		NR::InitializeCore();
		NR::Application* app = NR::CreateApplication(argc, argv);
		NR_CORE_ASSERT(app, "Client Application is null!");
		app->Run();
		delete app;
		NR::ShutdownCore();
	}
}
#else
#error NotRed only supports Windows
#endif