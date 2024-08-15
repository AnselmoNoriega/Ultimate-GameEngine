#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Core/Window.h"
#include "NotRed/Core/LayerStack.h"

#include "NotRed/Core/Events/ApplicationEvent.h"

#include "NotRed/ImGui/ImGuiLayer.h"

namespace NR
{
    class Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();

        virtual void Init() {}
        virtual void Update() {}
        virtual void Shutdown() {}
        
        void RenderImGui();

        virtual void OnEvent(Event& event);

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* layer);

        std::string OpenFile(const std::string& filter) const;

        inline Window& GetWindow() { return *mWindow; }

        static inline Application& Get() { return *sInstance; }

    private:
        bool OnWindowResize(WindowResizeEvent& e);
        bool OnWindowClose(WindowCloseEvent& e);

    private:
        static Application* sInstance;

        std::unique_ptr<Window> mWindow;
        bool mRunning = true;

        LayerStack mLayerStack;
        ImGuiLayer* mImGuiLayer;
    };

    Application* CreateApplication();
}