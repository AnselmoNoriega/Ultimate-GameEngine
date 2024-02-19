#pragma once

#include "NotRed/Window.h"

#include <GLFW/glfw3.h>

namespace NR
{
    class WindowsWindow : public Window
    {
    public:
        WindowsWindow(const WindowProps& props);
        virtual ~WindowsWindow();

        void Update() override;

        inline uint32_t GetWidth() const override { return mData.Width; }
        inline uint32_t GetHeight() const override { return mData.Height; }

        inline void SetEventCallback(const EventCallbackFn& callback) override { mData.EventCallback = callback; }
        void SetVSync(bool enabled) override;

        bool VSyncEnabled() const override { return mData.VSync; }

    private:
        virtual void Init(const WindowProps& props);
        virtual void Shutdown();

        virtual void EventCallBacks() const;

    private:
        GLFWwindow* mWindow;

        struct WindowData
        {
            std::string Tile;
            uint32_t Width, Height;
            bool VSync;

            EventCallbackFn EventCallback;
        };

        WindowData mData;
    };
}

