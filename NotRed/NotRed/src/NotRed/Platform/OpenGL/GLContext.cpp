#include "nrpch.h"
#include "GLContext.h"

#include <glad/glad.h>
#include <glfw/glfw3.h>

#include "NotRed/Core/Log.h"

namespace NR
{
	GLContext::GLContext(GLFWwindow* windowHandle)
		: mWindowHandle(windowHandle)
	{
	}

	GLContext::~GLContext()
	{
	}

	void GLContext::Create()
	{
		NR_CORE_INFO("GLContext::Create");

		glfwMakeContextCurrent(mWindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		NR_CORE_ASSERT(status, "Failed to initialize Glad!");

		NR_CORE_INFO("OpenGL Info:");
		NR_CORE_INFO("  Vendor: {0}", glGetString(GL_VENDOR));
		NR_CORE_INFO("  Renderer: {0}", glGetString(GL_RENDERER));
		NR_CORE_INFO("  Version: {0}", glGetString(GL_VERSION));

#ifdef NR_ENABLE_ASSERTS
		int versionMajor;
		int versionMinor;
		glGetIntegerv(GL_MAJOR_VERSION, &versionMajor);
		glGetIntegerv(GL_MINOR_VERSION, &versionMinor);

		NR_CORE_ASSERT(versionMajor > 4 || (versionMajor == 4 && versionMinor >= 5), "NotRed requires at least OpenGL version 4.5!");
#endif
	}

	void GLContext::SwapBuffers()
	{
		glfwSwapBuffers(mWindowHandle);
	}
}