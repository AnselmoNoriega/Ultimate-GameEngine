#pragma once

#include "Core.h"

namespace NR
{
	class NR_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
	};

	Application* CreateApplication();
}

