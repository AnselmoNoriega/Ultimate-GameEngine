#pragma once

#include "Core.h"

#include "Window.h"
#include "NotRed/LayerStack.h"

#include "NotRed/ImGui/ImGuiLayer.h"

#include "NotRed/Core/TimeStep.h"

namespace NR
{
    class WindowCloseEvent;

    class Application
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
        ImGuiLayer* mImGuiLayer;

        TimeStep mTimeStep;
    };

    Application* CreateApplication();
}

