#include "nrpch.h"
#include "Application.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "NotRed/Log.h"

#include "Input.h"

#include <glad/glad.h>

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
    }

    void Application::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));

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
            glClearColor(0.75f, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);

            for (Layer* layer : mLayerStack)
            {
                layer->Update();
            }

            mWindow->Update();
        }
    }

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        mRunning = false;
        return true;
    }

    Application::~Application()
    {
    }
}

