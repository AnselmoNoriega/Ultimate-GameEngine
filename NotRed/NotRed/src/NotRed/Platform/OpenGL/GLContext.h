#pragma once

#include "NotRed/Renderer/RendererContext.h"

struct GLFWwindow;

namespace NR
{
	class GLContext : public RendererContext
	{
	public:
		GLContext();
		~GLContext() override;

		void Init() override;
		
	private:
		GLFWwindow* mWindowHandle;
	};
}