#pragma once

#ifdef NR_PLATFORM_WINDOWS

extern NR::Application* NR::CreateApplication();

int main(int argc, char** argv)
{
	NR::Log::Initialize();
	NR_CORE_WARN("Initialized");
	int a = 10;
	NR_INFO("Var = {0}", a);

	auto app = NR::CreateApplication();
	app->Run();
	delete app;
	app = nullptr;

	return 0;
}

#else
	#error This Engine only supports Windows.
#endif