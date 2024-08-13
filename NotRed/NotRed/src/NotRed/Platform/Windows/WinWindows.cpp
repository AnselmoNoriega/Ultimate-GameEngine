#include "WinWindows.h"

#include "NotRed/Core/Log.h"

namespace NR
{
	static void GLFWErrorCallback(int error, const char* description)
	{
		NR_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

	static bool s_GLFWInitialized = false;

	Window* Window::Create(const WindowProps& props)
	{
		return new WinWindow(props);
	}

	WinWindow::WinWindow(const WindowProps& props)
	{
		Init(props);
	}

	WinWindow::~WinWindow()
	{
	}

	void WinWindow::Init(const WindowProps& props)
	{
		mData.Title = props.Title;
		mData.Width = props.Width;
		mData.Height = props.Height;

		NR_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

		if (!s_GLFWInitialized)
		{
			int success = glfwInit();
			NR_ASSERT(success, "Could not intialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);

			s_GLFWInitialized = true;
		}

		mWindow = glfwCreateWindow((int)props.Width, (int)props.Height, mData.Title.c_str(), nullptr, nullptr);
		glfwMakeContextCurrent(mWindow);
		glfwSetWindowUserPointer(mWindow, &mData);
		SetVSync(true);

		glfwSetWindowSizeCallback(mWindow, [](GLFWwindow* window, int width, int height)
			{
				auto& data = *((WindowData*)glfwGetWindowUserPointer(window));
				data.Width = width;
				data.Height = height;
			});
	}

	void WinWindow::Update()
	{
		NR_CORE_INFO("Window size = {0}, {1}", GetWidth(), GetHeight());
		glfwPollEvents();
		glfwSwapBuffers(mWindow);
	}

	void WinWindow::SetVSync(bool enabled)
	{
		if (enabled)
		{
			glfwSwapInterval(1);
		}

		else
		{
			glfwSwapInterval(0);
		}

		mData.VSync = enabled;
	}

	bool WinWindow::IsVSync() const
	{
		return mData.VSync;
	}
	}

	void WinWindow::Shutdown()
	{
	}
}