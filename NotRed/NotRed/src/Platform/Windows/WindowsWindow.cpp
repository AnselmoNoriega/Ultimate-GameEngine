#include "nrpch.h"
#include "WindowsWindow.h"

#include "NotRed/Events/ApplicationEvent.h"
#include "NotRed/Events/MouseEvent.h"
#include "NotRed/Events/KeyEvent.h"

#include "Platform/OpenGL/GLContext.h"

namespace NR
{
    static uint8_t sGLFWwindowCount = 0;

    static void GLFWErrorCallback(int error, const char* info)
    {
        NR_CORE_ERROR("GLFW ERRROR ({0}): {1}", error, info);
    }

    Scope<Window> Window::Create(const WindowProps& props)
    {
        return CreateScope<WindowsWindow>(props);
    }

    WindowsWindow::WindowsWindow(const WindowProps& props)
    {
        Init(props);
    }

    void WindowsWindow::Init(const WindowProps& props)
    {
        NR_PROFILE_FUNCTION();

        mData.Tile = props.Title;
        mData.Width = props.Width;
        mData.Height = props.Height;

        NR_CORE_INFO("Creating window \"{0}\" ({1}, {2})", props.Title, props.Width, props.Height);

        if (sGLFWwindowCount == 0)
        {
            NR_CORE_VERIFY(glfwInit(), "Could not initialize GLFW");
            glfwSetErrorCallback(GLFWErrorCallback);
        }

        mWindow = glfwCreateWindow(props.Width, props.Height, mData.Tile.c_str(), nullptr, nullptr);
        ++sGLFWwindowCount;

        mContext = new GLContext(mWindow);
        mContext->Init();

        glfwSetWindowUserPointer(mWindow, &mData);
        SetVSync(true);

        EventCallBacks();
    }

    void WindowsWindow::Update()
    {
        NR_PROFILE_FUNCTION();

        glfwPollEvents();
        mContext->SwapBuffers();
    }

    void WindowsWindow::SetVSync(bool enabled)
    {
        NR_PROFILE_FUNCTION();

        glfwSwapInterval(enabled ? 1 : 0);

        mData.VSync = enabled;
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

        glfwSetCharCallback(
            mWindow,
            [](GLFWwindow* window, unsigned int character)
            {
                WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                KeyTypedEvent charEvent(character);
                data.EventCallback(charEvent);
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
        NR_PROFILE_FUNCTION();

        glfwDestroyWindow(mWindow);

        --sGLFWwindowCount;

        if (sGLFWwindowCount == 0)
        {
            glfwTerminate();
        }
    }

    WindowsWindow::~WindowsWindow()
    {
        NR_PROFILE_FUNCTION();

        Shutdown();
    }
}

