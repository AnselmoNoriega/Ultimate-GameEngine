#include "nrpch.h"
#include "WindowsInput.h"

#include "NotRed/Core/Application.h"

#include <GLFW/glfw3.h>

namespace NR
{
    Input* Input::sInstance = new WindowsInput();

    bool WindowsInput::IsKeyPressedImpl(int keycode)
    {
        auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        auto state = glfwGetKey(window, keycode);

        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }
    bool WindowsInput::IsMouseBtnPressedImpl(int button)
    {
        auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        auto state = glfwGetMouseButton(window, button);

        return state == GLFW_PRESS;
    }
    std::pair<float, float> WindowsInput::GetMousePosImpl()
    {
        auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        double xPos, yPos;
        glfwGetCursorPos(window, &xPos, &yPos);

        return { (float)xPos, (float)yPos };
    }
}