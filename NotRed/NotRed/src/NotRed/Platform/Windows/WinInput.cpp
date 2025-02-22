#include "nrpch.h"
#include "NotRed/Core/Input.h"

#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "WinWindow.h"

#include "NotRed/Core/Application.h"


namespace NR
{
    void Input::Update()
    {
        // Cleanup disconnected controller
        for (auto it = sControllers.begin(); it != sControllers.end(); )
        {
            int id = it->first;
            if (glfwJoystickPresent(id) != GLFW_TRUE)
            {
                it = sControllers.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // Update controllers
        for (int id = GLFW_JOYSTICK_1; id < GLFW_JOYSTICK_LAST; id++)
        {
            if (glfwJoystickPresent(id) == GLFW_TRUE)
            {
                Controller& controller = sControllers[id];
                controller.ID = id;
                controller.Name = glfwGetJoystickName(id);

                int buttonCount;
                const unsigned char* buttons = glfwGetJoystickButtons(id, &buttonCount);
                for (int i = 0; i < buttonCount; i++)
                {
                    controller.ButtonStates[i] = buttons[i] == GLFW_PRESS;
                }

                int axisCount;
                const float* axes = glfwGetJoystickAxes(id, &axisCount);
                for (int i = 0; i < axisCount; i++)
                {
                    controller.AxisStates[i] = axes[i];
                }

                int hatCount;
                const unsigned char* hats = glfwGetJoystickHats(id, &hatCount);
                for (int i = 0; i < hatCount; i++)
                {
                    controller.HatStates[i] = hats[i];
                }
            }
        }
    }

    bool Input::IsKeyPressed(KeyCode keycode)
    {
        bool enableImGui = Application::Get().GetSpecification().EnableImGui;
        if (!enableImGui)
        {
            auto& window = static_cast<WinWindow&>(Application::Get().GetWindow());
            auto state = glfwGetKey(static_cast<GLFWwindow*>(window.GetNativeWindow()), static_cast<int32_t>(keycode));
            return state == GLFW_PRESS || state == GLFW_REPEAT;
        }

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
    }

    bool Input::IsMouseButtonPressed(MouseButton  button)
    {
        bool enableImGui = Application::Get().GetSpecification().EnableImGui;
        if (!enableImGui)
        {
            auto& window = static_cast<WinWindow&>(Application::Get().GetWindow());

            auto state = glfwGetMouseButton(static_cast<GLFWwindow*>(window.GetNativeWindow()), static_cast<int32_t>(button));
            return state == GLFW_PRESS;
        }

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

    bool Input::IsControllerPresent(int id)
    {
        return sControllers.find(id) != sControllers.end();
    }

    std::vector<int> Input::GetConnectedControllerIDs()
    {
        std::vector<int> ids;
        ids.reserve(sControllers.size());
        for (auto [id, controller] : sControllers)
        {
            ids.emplace_back(id);
        }

        return ids;
    }

    const Controller* Input::GetController(int id)
    {
        if (!Input::IsControllerPresent(id))
        {
            return nullptr;
        }

        return &sControllers.at(id);
    }

    std::string_view Input::GetControllerName(int id)
    {
        if (!Input::IsControllerPresent(id))
        {
            return {};
        }

        return sControllers.at(id).Name;
    }

    bool Input::IsControllerButtonPressed(int controllerID, int button)
    {
        if (!Input::IsControllerPresent(controllerID))
        {
            return false;
        }

        const Controller& controller = sControllers.at(controllerID);
        if (controller.ButtonStates.find(button) == controller.ButtonStates.end())
        {
            return false;
        }

        return controller.ButtonStates.at(button);
    }

    float Input::GetControllerAxis(int controllerID, int axis)
    {
        if (!Input::IsControllerPresent(controllerID))
        {
            return 0.0f;
        }

        const Controller& controller = sControllers.at(controllerID);
        if (controller.AxisStates.find(axis) == controller.AxisStates.end())
        {
            return 0.0f;
        }

        return controller.AxisStates.at(axis);
    }

    uint8_t Input::GetControllerHat(int controllerID, int hat)
    {
        if (!Input::IsControllerPresent(controllerID))
        {
            return 0;
        }

        const Controller& controller = sControllers.at(controllerID);
        if (controller.HatStates.find(hat) == controller.HatStates.end())
        {
            return 0;
        }

        return controller.HatStates.at(hat);
    }
}