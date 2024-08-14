#include "nrpch.h"
#include "Application.h"

#include "NotRed/Renderer/Renderer.h"
#include <GLFW/glfw3.h>

namespace NR
{
#define BIND_EVENT_FN(fn) std::bind(&Application::##fn, this, std::placeholders::_1)

    Application* Application::sInstance = nullptr;

    Application::Application()
    {
        sInstance = this;

        mWindow = std::unique_ptr<Window>(Window::Create());
        mWindow->SetEventCallback(BIND_EVENT_FN(OnEvent));

        mImGuiLayer = new ImGuiLayer("ImGui");
        PushOverlay(mImGuiLayer);

        Renderer::Init();
    }

    Application::~Application()
    {

    }

    void Application::RenderImGui()
    {
        mImGuiLayer->Begin();
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
            for (Layer* layer : mLayerStack)
            {
                layer->Update();
            }

            Application* app = this;
            NR_RENDER_1(app, { app->RenderImGui(); });

            Renderer::Get().WaitAndRender();

            mWindow->Update();
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
        return false;
    }

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        mRunning = false;
        return true;
    }
}