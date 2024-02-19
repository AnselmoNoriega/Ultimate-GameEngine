#include "nrpch.h"
#include "Application.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "NotRed/Log.h"

#include <GLFW/glfw3.h>

namespace NR
{
    Application::Application()
    {
        mWindow = std::unique_ptr<Window>(Window::Create());
    }

    Application::~Application()
    {
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
}

