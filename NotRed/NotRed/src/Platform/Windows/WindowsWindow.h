#pragma once

#include "NotRed/Window.h"
#include "NotRed/Renderer/GraphicsContext.h"

#include <GLFW/glfw3.h>

namespace NR
{
    class WindowsWindow : public Window
    {
    public:
        WindowsWindow(const WindowProps& props);
        ~WindowsWindow() override;

        void Update() override;

        inline uint32_t GetWidth() const override { return mData.Width; }
        inline uint32_t GetHeight() const override { return mData.Height; }

        inline void SetEventCallback(const EventCallbackFn& callback) override { mData.EventCallback = callback; }
        void SetVSync(bool enabled) override;

        bool VSyncEnabled() const override { return mData.VSync; }

        inline void* GetNativeWindow() const override { return mWindow; }

    private:
        void Init(const WindowProps& props);
        void Shutdown();

        void EventCallBacks() const;

    private:
        GLFWwindow* mWindow;
        GraphicsContext* mContext;

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

