#include "nrpch.h"
#include "WindowsWindow.h"

namespace NR
{
    static bool sGLFWInitialized = false;

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
            sGLFWInitialized = true;
        }

        mWindow = glfwCreateWindow(props.Width, props.Height, mData.Tile.c_str(), nullptr, nullptr);
        glfwMakeContextCurrent(mWindow);
        glfwSetWindowUserPointer(mWindow, &mData);
        SetVSync(true);
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

