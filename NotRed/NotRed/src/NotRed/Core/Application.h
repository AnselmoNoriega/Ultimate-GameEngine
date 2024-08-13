#pragma once

#include "NotRed/Core/Core.h"

namespace NR
{
	class NOT_RED_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
	};

	Application* CreateApplication();
}