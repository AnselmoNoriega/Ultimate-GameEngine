#pragma once

#include "Core.h"
#include "Window.h"

namespace NR
{
	class WindowCloseEvent;

	class NR_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

	private:
		bool OnWindowClose(WindowCloseEvent& e);

	private:
		bool mRunning = true;

		std::unique_ptr<Window> mWindow;
	};

	Application* CreateApplication();
}

