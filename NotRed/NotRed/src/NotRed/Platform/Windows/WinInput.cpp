#include "nrpch.h"
#include "NotRed/Core/Input.h"
#include "WinWindow.h"

#include "NotRed/Core/Application.h"

#include <GLFW/glfw3.h>

namespace NR
{
	bool Input::IsKeyPressed(int keycode)
	{
		auto& window = static_cast<WinWindow&>(Application::Get().GetWindow());

		auto state = glfwGetKey(static_cast<GLFWwindow*>(window.GetNativeWindow()), keycode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Input::IsMouseButtonPressed(int button)
	{
		auto& window = static_cast<WinWindow&>(Application::Get().GetWindow());

		auto state = glfwGetMouseButton(static_cast<GLFWwindow*>(window.GetNativeWindow()), button);
		return state == GLFW_PRESS;
	}

	float Input::GetMousePositionX()
	{
		auto& window = static_cast<WinWindow&>(Application::Get().GetWindow());

		double xpos, ypos;
		glfwGetCursorPos(static_cast<GLFWwindow*>(window.GetNativeWindow()), &xpos, &ypos);

		return (float)xpos;
	}

	float Input::GetMousePositionY()
	{
		auto& window = static_cast<WinWindow&>(Application::Get().GetWindow());

		double xpos, ypos;
		glfwGetCursorPos(static_cast<GLFWwindow*>(window.GetNativeWindow()), &xpos, &ypos);

		return (float)ypos;
	}
}