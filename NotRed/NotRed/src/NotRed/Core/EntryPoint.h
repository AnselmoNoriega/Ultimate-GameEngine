#pragma once
#include "NotRed/Core/Application.h"

#ifdef NR_PLATFORM_WINDOWS

extern NR::Application* NR::CreateApplication(NR::AppCommandLineArgs args);

int main(int argc, char** argv)
{
	NR::Log::Initialize();

	NR_PROFILE_BEGIN_SESSION("Startup", "Tracing/NotProfile-Startup.json");
	auto app = NR::CreateApplication({ argc, argv });
	NR_PROFILE_END_SESSION();


	NR_PROFILE_BEGIN_SESSION("Runtime", "Tracing/NotProfile-Runtime.json");
	app->Run();
	NR_PROFILE_END_SESSION();

	NR_PROFILE_BEGIN_SESSION("Shutdown", "Tracing/NotProfile-Shutdown.json");
	delete app;
	app = nullptr;
	NR_PROFILE_END_SESSION();

	return 0;
}

#else
	#error This Engine only supports Windows.
#endif