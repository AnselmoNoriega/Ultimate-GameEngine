#include "nrpch.h"
#include "NotRed/Core/Input.h"

#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "WinWindow.h"

#include "NotRed/Core/Application.h"


namespace NR
{
#define SINGLE_WINDOW 0

	bool Input::IsKeyPressed(KeyCode keycode)
	{
#if SINGLE_WINDOW
		auto& window = static_cast<WinWindow&>(Application::Get().GetWindow());

		auto state = glfwGetKey(static_cast<GLFWwindow*>(window.GetNativeWindow()), static_cast<int32_t>(keycode));
		return state == GLFW_PRESS || state == GLFW_REPEAT;
#else
		auto& window = static_cast<WinWindow&>(Application::Get().GetWindow());
		GLFWwindow* win = static_cast<GLFWwindow*>(window.GetNativeWindow());
		ImGuiContext* context = ImGui::GetCurrentContext();
		bool pressed = false;

		for (ImGuiViewport* viewport : context->Viewports)
		{
			if (!viewport->PlatformUserData)
			{
				continue;
			}

			GLFWwindow* windowHandle = *(GLFWwindow**)viewport->PlatformUserData; // First member is GLFWwindow
			if (!windowHandle)
			{
				continue;
			}

			auto state = glfwGetKey(windowHandle, static_cast<int32_t>(keycode));
			if (state == GLFW_PRESS || state == GLFW_REPEAT)
			{
				pressed = true;
				break;
			}
		}
		return pressed;
#endif
	}

	bool Input::IsMouseButtonPressed(MouseButton  button)
	{
#if SINGLE_WINDOW
		auto& window = static_cast<WinWindow&>(Application::Get().GetWindow());

		auto state = glfwGetMouseButton(static_cast<GLFWwindow*>(window.GetNativeWindow()), static_cast<int32_t>(button));
		return state == GLFW_PRESS;
#else
		ImGuiContext* context = ImGui::GetCurrentContext();
		bool pressed = false;

		for (ImGuiViewport* viewport : context->Viewports)
		{
			if (!viewport->PlatformUserData)
			{
				continue;
			}

			GLFWwindow* windowHandle = *(GLFWwindow**)viewport->PlatformUserData; // First member is GLFWwindow
			if (!windowHandle)
			{
				continue;
			}

			auto state = glfwGetMouseButton(static_cast<GLFWwindow*>(windowHandle), static_cast<int32_t>(button));
			if (state == GLFW_PRESS || state == GLFW_REPEAT)
			{
				pressed = true;
				break;
			}
		}
		return pressed;
#endif
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

	void Input::SetCursorMode(CursorMode mode)
	{
		auto& window = static_cast<WinWindow&>(Application::Get().GetWindow());
		glfwSetInputMode(static_cast<GLFWwindow*>(window.GetNativeWindow()), GLFW_CURSOR, GLFW_CURSOR_NORMAL + (int)mode);
	}

	CursorMode Input::GetCursorMode()
	{
		auto& window = static_cast<WinWindow&>(Application::Get().GetWindow());
		return (CursorMode)(glfwGetInputMode(static_cast<GLFWwindow*>(window.GetNativeWindow()), GLFW_CURSOR) - GLFW_CURSOR_NORMAL);
	}
}