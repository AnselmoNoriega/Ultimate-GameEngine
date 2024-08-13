#pragma once

#ifdef NR_PLATFORM_WINDOWS

extern NR::Application* NR::CreateApplication();

int main(int argc, char** argv)
{
	NR::Application* app = NR::CreateApplication();
	app->Run();
	delete app;
}
#else
#error NotRed only supports Windows
#endif