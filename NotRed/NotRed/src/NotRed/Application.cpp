#include "nrpch.h"
#include "Application.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "NotRed/Log.h"

#include <GLFW/glfw3.h>

namespace NR
{
#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

    Application::Application()
    {
        mWindow = std::unique_ptr<Window>(Window::Create());
        mWindow->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));
    }

    void Application::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
        NR_CORE_INFO("{0}", e);
    }

    void Application::Run()
    {
        while (mRunning)
        {   
            glClearColor(0.75f, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);
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

