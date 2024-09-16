#pragma once

#include "NotRed/Renderer/RendererContext.h"

struct GLFWwindow;

namespace NR
{
	class GLContext : public RendererContext
	{
	public:
		GLContext(GLFWwindow* windowHandle);
		~GLContext() override;

		void Create() override;
		void BeginFrame() override {}
		void SwapBuffers() override;
		void Resize(uint32_t width, uint32_t height) override {}
		
	private:
		GLFWwindow* mWindowHandle;
	};
}