#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Core/Window.h"

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

    private:
        std::unique_ptr<Window> mWindow;
    };

    Application* CreateApplication();
}