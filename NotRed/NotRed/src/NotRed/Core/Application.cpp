#include "nrpch.h"
#include "Application.h"

#include "NotRed/Core/Log.h"

#include "NotRed/Events/Event.h"
#include "NotRed/Events/ApplicationEvent.h"

#include "Input.h"

#include "NotRed/Renderer/Renderer.h"

#include <GLFW/glfw3.h>

namespace NR
{
#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

    Application* Application::sInstance = nullptr;

    Application::Application()
    {
        NR_CORE_ASSERT(!sInstance, "There can only be one application");
        sInstance = this;

        mWindow = std::unique_ptr<Window>(Window::Create());
        mWindow->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

        Renderer::Init();

        mImGuiLayer = new ImGuiLayer();
        PushOverlay(mImGuiLayer);
    }

    void Application::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));

        for (auto layer = mLayerStack.end(); layer != mLayerStack.begin();)
        {
            (*--layer)->OnEvent(e);
            if (e.Handled)
            {
                break;
            }
        }
    }

    void Application::PushLayer(Layer* layer)
    {
        mLayerStack.PushLayer(layer);
        layer->Attach();
    }

    void Application::PushOverlay(Layer* overlay)
    {
        mLayerStack.PushOverlay(overlay);
        overlay->Attach();
    }

    void Application::Run()
    {
        while (mRunning)
        {
            float time = static_cast<float>(glfwGetTime());
            mTimeStep.UpdateTimeFrame(time);

            if (!mMinimized)
            {
                for (Layer* layer : mLayerStack)
                {
                    layer->Update(mTimeStep);
                }
            }

            mImGuiLayer->Begin();
            for (Layer* layer : mLayerStack)
            {
                layer->ImGuiRender();
            }
            mImGuiLayer->End();

            mWindow->Update();
        }
    }

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        mRunning = false;
        return true;
    }

    bool Application::OnWindowResize(WindowResizeEvent& e)
    {
        if (e.GetWidth() == 0 || e.GetHeight() == 0)
        {
            mMinimized = true;
            return false;
        }

        mMinimized = false;
        Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

        return false;
    }

    Application::~Application()
    {
    }
}

