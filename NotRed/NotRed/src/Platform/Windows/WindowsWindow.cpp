#include "nrpch.h"
#include "WindowsWindow.h"

#include "NotRed/Events/ApplicationEvent.h"
#include "NotRed/Events/MouseEvent.h"
#include "NotRed/Events/KeyEvent.h"

#include <glad/glad.h>

namespace NR
{
    static bool sGLFWInitialized = false;

    static void GLFWErrorCallback(int error, const char* info)
    {
        NR_CORE_ERROR("GLFW ERRROR ({0}): {1}", error, info);
    }

    Window* Window::Create(const WindowProps& props)
    {
        return new WindowsWindow(props);
    }

    WindowsWindow::WindowsWindow(const WindowProps& props)
    {
        Init(props);
    }

    void WindowsWindow::Update()
    {
        glfwPollEvents();
        glfwSwapBuffers(mWindow);
    }

    void WindowsWindow::SetVSync(bool enabled)
    {
        glfwSwapInterval(enabled ? 1 : 0);

        mData.VSync = enabled;
    }

    void WindowsWindow::Init(const WindowProps& props)
    {
        mData.Tile = props.Title;
        mData.Width = props.Width;
        mData.Height = props.Height;

        NR_CORE_INFO("Creating window \"{0}\" ({1}, {2})", props.Title, props.Width, props.Height);

        if (!sGLFWInitialized)
        {
            NR_CORE_VERIFY(glfwInit(), "Could not initialize GLFW");
            glfwSetErrorCallback(GLFWErrorCallback);
            sGLFWInitialized = true;
        }

        mWindow = glfwCreateWindow(props.Width, props.Height, mData.Tile.c_str(), nullptr, nullptr);
        glfwMakeContextCurrent(mWindow);
        int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        NR_CORE_ASSERT(status, "Glad failed to initialize");
        glfwSetWindowUserPointer(mWindow, &mData);
        SetVSync(true);

        EventCallBacks();
    }

    void WindowsWindow::EventCallBacks() const
    {
        glfwSetWindowSizeCallback(
            mWindow,
            [](GLFWwindow* window, int width, int height)
            {
                WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                data.Width = width;
                data.Height = height;

                WindowResizeEvent winEvent(width, height);
                data.EventCallback(winEvent);
            }
        );

        glfwSetWindowCloseCallback(
            mWindow,
            [](GLFWwindow* window)
            {
                WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                WindowCloseEvent winEvent;
                data.EventCallback(winEvent);
            }
        );

        glfwSetKeyCallback(
            mWindow,
            [](GLFWwindow* window, int key, int scancode, int action, int mods)
            {
                WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

                switch (action)
                {
                case GLFW_PRESS:
                {
                    KeyPressedEvent keyEvent(key, 0);
                    data.EventCallback(keyEvent);
                    break;
                }
                case GLFW_RELEASE:
                {
                    KeyReleasedEvent keyEvent(key);
                    data.EventCallback(keyEvent);
                    break;
                }
                case GLFW_REPEAT:
                {
                    KeyPressedEvent keyEvent(key, 1);
                    data.EventCallback(keyEvent);
                    break;
                }
                }
            }
        );

        glfwSetMouseButtonCallback(
            mWindow,
            [](GLFWwindow* window, int button, int action, int mods)
            {
                WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

                switch (action)
                {
                case GLFW_PRESS:
                {
                    MouseButtonPressedEvent btnEvent(button);
                    data.EventCallback(btnEvent);
                    break;
                }
                case GLFW_RELEASE:
                {
                    MouseButtonReleasedEvent btnEvent(button);
                    data.EventCallback(btnEvent);
                    break;
                }
                }
            }
        );

        glfwSetScrollCallback(
            mWindow,
            [](GLFWwindow* window, double offsetX, double offsetY)
            {
                WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

                MouseScrolledEvent scrlEvent((float)offsetX, (float)offsetY);
                data.EventCallback(scrlEvent);
            }
        );

        glfwSetCursorPosCallback(
            mWindow,
            [](GLFWwindow* window, double posX, double posY)
            {
                WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

                MouseMovedEvent moveEvent((float)posX, (float)posY);
                data.EventCallback(moveEvent);
            }
        );
    }

    void WindowsWindow::Shutdown()
    {
        glfwDestroyWindow(mWindow);
    }

    WindowsWindow::~WindowsWindow()
    {
        Shutdown();
    }
}

