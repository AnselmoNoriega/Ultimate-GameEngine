#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Core/Window.h"
#include "NotRed/Core/TimeFrame.h"
#include "NotRed/Core/Timer.h"
#include "NotRed/Core/LayerStack.h"

#include "NotRed/Core/Events/ApplicationEvent.h"

#include "NotRed/ImGui/ImGuiLayer.h"

namespace NR
{
    struct ApplicationSpecification
    {
        std::string Name = "NotRed";

        uint32_t WindowWidth = 1600, WindowHeight = 900;
        bool Fullscreen = false;
        bool VSync = true;
        std::string WorkingDirectory;
        bool StartMaximized = true;
        bool Resizable = true;
        bool EnableImGui = true;
    };

    class Application
    {
    public:
        Application(const ApplicationSpecification& specification);
        virtual ~Application();

        void Run();
        void Close();

        virtual void Init() {}
        virtual void Update(float dt) {}
        virtual void Shutdown();
        
        void RenderImGui();

        virtual void OnEvent(Event& event);

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* layer);
        void PopLayer(Layer* layer);
        void PopOverlay(Layer* layer);

        std::string OpenFile(const char* filter = "All\0*.*\0") const;
        std::string OpenFolder(const char* initialFolder = "") const;
        std::string SaveFile(const char* filter = "All\0*.*\0") const;

        void SetShowStats(bool show) { mShowStats = show; }

        inline Window& GetWindow() { return *mWindow; }

        static inline Application& Get() { return *sInstance; }

        float GetTime() const;
        float GetTimeFrame() const;

        static const char* GetConfigurationName();
        static const char* GetPlatformName();

        const ApplicationSpecification& GetSpecification() const { return mSpecification; }

        PerformanceProfiler* GetPerformanceProfiler() { return mProfiler; }

    private:
        bool OnWindowResize(WindowResizeEvent& e);
        bool OnWindowClose(WindowCloseEvent& e);

    private:
        static Application* sInstance;

        std::unique_ptr<Window> mWindow;

        ApplicationSpecification mSpecification;

        bool mRunning = true, mMinimized = false;

        LayerStack mLayerStack;
        ImGuiLayer* mImGuiLayer;

        TimeFrame mTimeFrame;
        float mLastFrameTime = 0.0f;
        bool mEnableImGui = true;

        PerformanceProfiler* mProfiler = nullptr;
        bool mShowStats = true;
    };

    Application* CreateApplication(int argc, char** argv);
}