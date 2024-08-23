#include "nrpch.h"
#include "NotRed/Core/Input.h"
#include "WinWindow.h"

#include "NotRed/Core/Application.h"

#include <GLFW/glfw3.h>

namespace NR
{
	bool Input::IsKeyPressed(KeyCode keycode)
	{
		auto& window = static_cast<WinWindow&>(Application::Get().GetWindow());

		auto state = glfwGetKey(static_cast<GLFWwindow*>(window.GetNativeWindow()), static_cast<int32_t>(keycode));
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
		auto [x, y] = GetMousePosition();
		return (float)x;
	}

	float Input::GetMousePositionY()
	{
		auto [x, y] = GetMousePosition();
		return (float)y;
	}

	std::pair<float, float> Input::GetMousePosition()
	{
		auto& window = static_cast<WinWindow&>(Application::Get().GetWindow());

		double x, y;
		glfwGetCursorPos(static_cast<GLFWwindow*>(window.GetNativeWindow()), &x, &y);
		return { (float)x, (float)y };
	}
}