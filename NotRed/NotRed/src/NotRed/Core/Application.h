#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Core/Window.h"

#include "NotRed/Core/Events/ApplicationEvent.h"

namespace NR
{
    class NOT_RED_API Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();

        virtual void Init() {}
        virtual void Update() {}
        virtual void Shutdown() {}

        virtual void OnEvent(Event& event);

    private:
        bool OnWindowResize(WindowResizeEvent& e);
        bool OnWindowClose(WindowCloseEvent& e);

    private:
        std::unique_ptr<Window> mWindow;
        bool mRunning = true;
    };

    Application* CreateApplication();
}