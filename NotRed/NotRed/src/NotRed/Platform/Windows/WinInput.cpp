#include "nrpch.h"
#include "WinInput.h"
#include "WinWindow.h"

#include "NotRed/Core/Application.h"

#include <GLFW/glfw3.h>

namespace NR
{
	bool WindowsInput::IsKeyPressedImpl(int keycode)
	{
		auto& window = static_cast<WinWindow&>(Application::Get().GetWindow());

		auto state = glfwGetKey(static_cast<GLFWwindow*>(window.GetNativeWindow()), keycode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool WindowsInput::IsMouseButtonPressedImpl(int button)
	{
		auto& window = static_cast<WinWindow&>(Application::Get().GetWindow());

		auto state = glfwGetMouseButton(static_cast<GLFWwindow*>(window.GetNativeWindow()), button);
		return state == GLFW_PRESS;
	}

	float WindowsInput::GetMouseXImpl()
	{
		auto& window = static_cast<WinWindow&>(Application::Get().GetWindow());

		double xpos, ypos;
		glfwGetCursorPos(static_cast<GLFWwindow*>(window.GetNativeWindow()), &xpos, &ypos);

		return (float)xpos;
	}

	float WindowsInput::GetMouseYImpl()
	{
		auto& window = static_cast<WinWindow&>(Application::Get().GetWindow());

		double xpos, ypos;
		glfwGetCursorPos(static_cast<GLFWwindow*>(window.GetNativeWindow()), &xpos, &ypos);

		return (float)ypos;
	}
}