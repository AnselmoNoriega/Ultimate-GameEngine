#include "nrpch.h"
#include "Application.h"

#include "NotRed/Renderer/Renderer.h"
#include "NotRed/Renderer/Framebuffer.h"
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>

#include "NotRed/Script/ScriptEngine.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <Windows.h>

namespace NR
{
#define BIND_EVENT_FN(fn) std::bind(&Application::##fn, this, std::placeholders::_1)

    Application* Application::sInstance = nullptr;

    Application::Application(const ApplicationProps& props)
    {
        sInstance = this;

        mWindow = std::unique_ptr<Window>(Window::Create(WindowProps(props.Name, props.WindowWidth, props.WindowHeight)));
        mWindow->SetEventCallback(BIND_EVENT_FN(OnEvent));
        mWindow->SetVSync(true);

        mImGuiLayer = new ImGuiLayer("ImGui");
        PushOverlay(mImGuiLayer);

        ScriptEngine::Init("Assets/Scripts/ExampleApp.dll");

        Renderer::Init();
        Renderer::WaitAndRender();
    }

    Application::~Application()
    {
        ScriptEngine::Shutdown();
    }

    void Application::RenderImGui()
    {
        mImGuiLayer->Begin();

        ImGui::Begin("Renderer");
        auto& caps = RendererAPI::GetCapabilities();
        ImGui::Text("Vendor: %s", caps.Vendor.c_str());
        ImGui::Text("Renderer: %s", caps.Renderer.c_str());
        ImGui::Text("Version: %s", caps.Version.c_str());
        ImGui::Text("Frame Time: %.2fms\n", mTimeFrame.GetMilliseconds());
        ImGui::End();

        for (Layer* layer : mLayerStack)
        {
            layer->ImGuiRender();
        }

        mImGuiLayer->End();
    }

    void Application::PushLayer(Layer* layer)
    {
        mLayerStack.PushLayer(layer);
        layer->Attach();
    }

    void Application::PushOverlay(Layer* layer)
    {
        mLayerStack.PushOverlay(layer);
        layer->Attach();
    }

    void Application::Run()
    {
        Init();

        while (mRunning)
        {
            if (!mMinimized)
            {
                for (Layer* layer : mLayerStack)
                {
                    layer->Update((float)mTimeFrame);
                }
                Application* app = this;
                Renderer::Submit([app]() { app->RenderImGui(); });

                Renderer::WaitAndRender();
            }

            mWindow->Update();

            float time = GetTime();
            mTimeFrame = time - mLastFrameTime;
            mLastFrameTime = time;
        }

        Shutdown();
    }

    void Application::OnEvent(Event& event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(OnWindowResize));
        dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));

        for (auto it = mLayerStack.end(); it != mLayerStack.begin(); )
        {
            (*--it)->OnEvent(event);
            if (event.Handled)
            {
                break;
            }
        }
    }

    bool Application::OnWindowResize(WindowResizeEvent& e)
    {
        int width = e.GetWidth(), height = e.GetHeight();
        if (width == 0 || height == 0)
        {
            mMinimized = true;
            return false;
        }
        mMinimized = false;

        Renderer::Submit([=]() { glViewport(0, 0, width, height); });
        auto& fbs = FrameBufferPool::GetGlobal()->GetAll();
        for (auto& fb : fbs)
        {
            fb->Resize(width, height);
        }

        return false;
    }

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        mRunning = false;
        return true;
    }

    std::string Application::OpenFile(const char* filter) const
    {
        OPENFILENAMEA ofn;
        CHAR szFile[260] = { 0 };

        // Initialize OPENFILENAME
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)mWindow->GetNativeWindow());
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn) == TRUE)
        {
            return ofn.lpstrFile;
        }
        return std::string();
    }

    std::string Application::SaveFile(const char* filter) const
    {
        OPENFILENAMEA ofn;       // common dialog box structure
        CHAR szFile[260] = { 0 };       // if using TCHAR macros

        // Initialize OPENFILENAME
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)mWindow->GetNativeWindow());
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetSaveFileNameA(&ofn) == TRUE)
        {
            return ofn.lpstrFile;
        }
        return std::string();
    }

    float Application::GetTime() const
    {
        return (float)glfwGetTime();
    }
}