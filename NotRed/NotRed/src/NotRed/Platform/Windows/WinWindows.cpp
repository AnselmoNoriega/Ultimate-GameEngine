#include "nrpch.h"
#include "WinWindows.h"

#include "NotRed/Core/Events/ApplicationEvent.h"

#include "NotRed/Core/Events/KeyEvent.h"
#include "NotRed/Core/Events/MouseEvent.h"

namespace NR
{
    static bool sGLFWInitialized = false;

    static void GLFWErrorCallback(int error, const char* description)
    {
        NR_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
    }

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

        if (!sGLFWInitialized)
        {
            int success = glfwInit();
            NR_CORE_ASSERT(success, "Could not intialize GLFW!");
            glfwSetErrorCallback(GLFWErrorCallback);

            sGLFWInitialized = true;
        }

        mWindow = glfwCreateWindow((uint32_t)props.Width, (uint32_t)props.Height, mData.Title.c_str(), nullptr, nullptr);
        glfwMakeContextCurrent(mWindow);
        glfwSetWindowUserPointer(mWindow, &mData);
        SetVSync(true);

        glfwSetWindowSizeCallback(mWindow, [](GLFWwindow* window, int width, int height)
            {
                auto& data = *((WindowData*)glfwGetWindowUserPointer(window));
                data.Width = width;
                data.Height = height;

                WindowResizeEvent winEvent((uint32_t)width, (uint32_t)height);
                data.EventCallback(winEvent);
            });

        glfwSetWindowCloseCallback(mWindow, [](GLFWwindow* window)
            {
                auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

                WindowCloseEvent winEvent;
                data.EventCallback(winEvent);
            });

        glfwSetKeyCallback(mWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods)
            {
                auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

                switch (action)
                {
                case GLFW_PRESS:
                {
                    KeyPressedEvent winEvent(key, 0);
                    data.EventCallback(winEvent);
                    break;
                }
                case GLFW_RELEASE:
                {
                    KeyReleasedEvent winEvent(key);
                    data.EventCallback(winEvent);
                    break;
                }
                case GLFW_REPEAT:
                {
                    KeyPressedEvent winEvent(key, 1);
                    data.EventCallback(winEvent);
                    break;
                }
                }
            });

        glfwSetMouseButtonCallback(mWindow, [](GLFWwindow* window, int button, int action, int mods)
            {
                auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

                switch (action)
                {
                case GLFW_PRESS:
                {
                    MouseButtonPressedEvent winEvent(button);
                    data.EventCallback(winEvent);
                    break;
                }
                case GLFW_RELEASE:
                {
                    MouseButtonReleasedEvent winEvent(button);
                    data.EventCallback(winEvent);
                    break;
                }
                }
            });

        glfwSetScrollCallback(mWindow, [](GLFWwindow* window, double xOffset, double yOffset)
            {
                auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

                MouseScrolledEvent winEvent((float)xOffset, (float)yOffset);
                data.EventCallback(winEvent);
            });

        glfwSetCursorPosCallback(mWindow, [](GLFWwindow* window, double x, double y)
            {
                auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

                MouseMovedEvent winEvent((float)x, (float)y);
                data.EventCallback(winEvent);
            });
    }

    void WinWindow::Update()
    {
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

    void WinWindow::Shutdown()
    {
    }
}