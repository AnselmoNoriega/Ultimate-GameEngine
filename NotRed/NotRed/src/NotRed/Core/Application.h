#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Core/Window.h"
#include "NotRed/Core/TimeFrame.h"
#include "NotRed/Core/LayerStack.h"

#include "NotRed/Core/Events/ApplicationEvent.h"

#include "NotRed/ImGui/ImGuiLayer.h"

namespace NR
{
    struct ApplicationProps
    {
        std::string Name;
        uint32_t WindowWidth, WindowHeight;
    };

    class Application
    {
    public:
        Application(const ApplicationProps& props = { "NotRed Engine", 1280, 720 });
        virtual ~Application();

        void Run();

        virtual void Init() {}
        virtual void Update(float dt) {}
        virtual void Shutdown() {}
        
        void RenderImGui();

        virtual void OnEvent(Event& event);

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* layer);

        std::string OpenFile(const char* filter = "All\0*.*\0") const;
        std::string SaveFile(const char* filter = "All\0*.*\0") const;

        inline Window& GetWindow() { return *mWindow; }

        static inline Application& Get() { return *sInstance; }

        float GetTime() const;

        static const char* GetConfigurationName();
        static const char* GetPlatformName();

    private:
        bool OnWindowResize(WindowResizeEvent& e);
        bool OnWindowClose(WindowCloseEvent& e);

    private:
        static Application* sInstance;

        std::unique_ptr<Window> mWindow;
        bool mRunning = true, mMinimized = false;

        LayerStack mLayerStack;
        ImGuiLayer* mImGuiLayer;

        TimeFrame mTimeFrame;
        float mLastFrameTime = 0.0f;
    };

    Application* CreateApplication();
}