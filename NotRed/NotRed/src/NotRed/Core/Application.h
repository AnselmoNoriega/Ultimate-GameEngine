#pragma once

#include "Core.h"

#include "Window.h"
#include "NotRed/Core/LayerStack.h"

#include "NotRed/ImGui/ImGuiLayer.h"

#include "NotRed/Core/TimeStep.h"

namespace NR
{
    class WindowCloseEvent;
    class WindowResizeEvent;

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
        bool OnWindowResize(WindowResizeEvent& e);

    private:
        static Application* sInstance;

        bool mRunning = true;
        bool mMinimized = false;

        std::unique_ptr<Window> mWindow;
        
        LayerStack mLayerStack;
        ImGuiLayer* mImGuiLayer;

        TimeStep mTimeStep;
    };

    Application* CreateApplication();
}

