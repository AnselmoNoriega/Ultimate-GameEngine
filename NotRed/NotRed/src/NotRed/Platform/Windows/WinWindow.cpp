#include "nrpch.h"
#include "WinWindow.h"

#include <glad/glad.h>
#include <imgui.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "stb_image.h"

#include "NotRed/Core/Events/ApplicationEvent.h"

#include "NotRed/Core/Events/KeyEvent.h"
#include "NotRed/Core/Events/MouseEvent.h"

#include "NotRed/Renderer/RendererAPI.h"

#include "NotRed/Platform/Vulkan/VKContext.h"

namespace NR
{
    static bool sGLFWInitialized = false;

    static void GLFWErrorCallback(int error, const char* description)
    {
        NR_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
    }

    Window* Window::Create(const WindowSpecification& specification)
    {
        return new WinWindow(specification);
    }

    WinWindow::WinWindow(const WindowSpecification& props)
        : mSpecification(props)
    {}

    WinWindow::~WinWindow()
    {
        Shutdown();
    }

    void WinWindow::Init()
    {
        mData.Title = mSpecification.Title;
        mData.Width = mSpecification.Width;
        mData.Height = mSpecification.Height;

        NR_CORE_INFO("Creating window {0} ({1}, {2})", mSpecification.Title, mSpecification.Width, mSpecification.Height);

        if (!sGLFWInitialized)
        {
            int success = glfwInit();
            NR_CORE_ASSERT(success, "Could not intialize GLFW!");
            glfwSetErrorCallback(GLFWErrorCallback);

            sGLFWInitialized = true;
        }

        if (RendererAPI::Current() == RendererAPIType::Vulkan)
        {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        }

#ifdef NR_PLATFORM_WINDOWS
        glfwWindowHint(GLFW_TITLEBAR, false);
#else
        glfwWindowHint(GLFW_DECORATED, false);
#endif

        if (mSpecification.Fullscreen)
        {
            GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
            mWindow = glfwCreateWindow(mode->width, mode->height, mData.Title.c_str(), primaryMonitor, nullptr);
        }
        else
        {
            mWindow = glfwCreateWindow((int)mSpecification.Width, (int)mSpecification.Height, mData.Title.c_str(), nullptr, nullptr);
        }

        mRendererContext = RendererContext::Create();
        mRendererContext->Init();

        Ref<VKContext> context = mRendererContext.As<VKContext>();
        mSwapChain.Init(VKContext::GetInstance(), context->GetDevice());
        mSwapChain.InitSurface(mWindow);

        uint32_t width = mData.Width, height = mData.Height;
        mSwapChain.Create(&width, &height, mSpecification.VSync);

        glfwSetWindowUserPointer(mWindow, &mData);

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
                    KeyPressedEvent winEvent((KeyCode)key, 0);
                    data.EventCallback(winEvent);
                    break;
                }
                case GLFW_RELEASE:
                {
                    KeyReleasedEvent winEvent((KeyCode)key);
                    data.EventCallback(winEvent);
                    break;
                }
                case GLFW_REPEAT:
                {
                    KeyPressedEvent winEvent((KeyCode)key, 1);
                    data.EventCallback(winEvent);
                    break;
                }
                }
            });

        glfwSetCharCallback(mWindow, [](GLFWwindow* window, uint32_t codepoint)
            {
                auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

                KeyTypedEvent event((KeyCode)codepoint);
                data.EventCallback(event);
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

        glfwSetTitlebarHitTestCallback(mWindow, [](GLFWwindow* window, int x, int y, int* hit)
            {
                auto& data = *((WindowData*)glfwGetWindowUserPointer(window));
                WindowTitleBarHitTestEvent event(x, y, *hit);
                data.EventCallback(event);
            });

        mImGuiMouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        mImGuiMouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
        mImGuiMouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        mImGuiMouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
        mImGuiMouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
        mImGuiMouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        mImGuiMouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        mImGuiMouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

        {
            int width, height;
            glfwGetWindowSize(mWindow, &width, &height);
            mData.Width = width;
            mData.Height = height;
        }

        // Set window icon
        {
            GLFWimage icon;
            int channels;
            icon.pixels = stbi_load(
                "Resources/Editor/NR_logo_square.png",
                &icon.width, &icon.height, &channels, 4
            );
            glfwSetWindowIcon(mWindow, 1, &icon);

            /*{
                HICON hIcon = CreateIcon(
                    nullptr,                      // No specific instance handle
                    icon.width,                 // Icon width
                    icon.height,                // Icon height
                    1,                            // Color planes
                    32,                           // Bits per pixel
                    nullptr,                      // AND mask (null for full transparency)
                    icon.pixels                 // XOR mask (image data in RGBA format)
                );

                if (!hIcon)
                {
                    stbi_image_free(icon.pixels);
                    return;
                }

                // Get the native HWND from the GLFW window
                HWND hwnd = glfwGetWin32Window(mWindow);

                // Set the large taskbar icon
                SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

                // Optionally, set the small title bar icon
                SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            }*/ 

            stbi_image_free(icon.pixels);
        }
    }

    inline std::pair<float, float> WinWindow::GetWindowPos() const
    {
        int x, y;
        glfwGetWindowPos(mWindow, &x, &y);
        return { (float)x, (float)y };
    }

    void WinWindow::ProcessEvents()
    {
        glfwPollEvents();
    }

    void WinWindow::SwapBuffers()
    {
        mSwapChain.Present();
    }

    void WinWindow::SetVSync(bool enabled)
    {
        mData.VSync = enabled;
    }

    void WinWindow::Maximize()
    {
        glfwMaximizeWindow(mWindow);
    }

    void WinWindow::CenterWindow()
    {
        const GLFWvidmode* videmode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        int x = (videmode->width / 2) - (mData.Width / 2);
        int y = (videmode->height / 2) - (mData.Height / 2);
        glfwSetWindowPos(mWindow, x, y);
    }

    bool WinWindow::IsVSync() const
    {
        return mData.VSync;
    }

    void WinWindow::SetResizable(bool resizable) const
    {
        glfwSetWindowAttrib(mWindow, GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
    }

    void WinWindow::Shutdown()
    {
        glfwTerminate();
        sGLFWInitialized = false;
    }

    void WinWindow::SetTitle(const std::string& title)
    {
        mData.Title = title;
        glfwSetWindowTitle(mWindow, mData.Title.c_str());
    }

    VKSwapChain& WinWindow::GetSwapChain()
    {
        return mSwapChain;
    }
}