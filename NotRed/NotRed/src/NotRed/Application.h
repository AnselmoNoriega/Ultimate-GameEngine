#pragma once

#include "Core.h"

#include "Window.h"
#include "NotRed/LayerStack.h"

namespace NR
{
    class WindowCloseEvent;

    class NR_API Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();

        void OnEvent(Event& e);

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* overlay);

    private:
        bool OnWindowClose(WindowCloseEvent& e);

    private:
        bool mRunning = true;

        std::unique_ptr<Window> mWindow;
        
        LayerStack mLayerStack;
    };

    Application* CreateApplication();
}

