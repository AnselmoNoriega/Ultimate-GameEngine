#pragma once

#include "Core.h"
#include "Window.h"

namespace NR
{
	class NR_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

	private:
		bool mRunning = true;

		std::unique_ptr<Window> mWindow;
	};

	Application* CreateApplication();
}

