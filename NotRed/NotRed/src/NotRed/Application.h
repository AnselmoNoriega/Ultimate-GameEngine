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

        inline Window& GetWindow() { return *mWindow; }
        inline static Application& Get() { return *sInstance; }

    private:
        bool OnWindowClose(WindowCloseEvent& e);

    private:
        static Application* sInstance;

        bool mRunning = true;

        std::unique_ptr<Window> mWindow;
        
        LayerStack mLayerStack;
    };

    Application* CreateApplication();
}

