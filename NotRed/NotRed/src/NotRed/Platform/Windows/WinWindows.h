#pragma once

#include <GLFW/glfw3.h>

#include "NotRed/Core/Window.h"

namespace NR
{
    class WinWindow : public Window
    {
    public:
        WinWindow(const WindowProps& props);
        ~WinWindow() override;

        void Update() override;

        inline uint32_t GetWidth() const override { return mData.Width; }
        inline uint32_t GetHeight() const override { return mData.Height; }

        inline void SetEventCallback(const EventCallbackFn& callback) override { mData.EventCallback = callback; }
        void SetVSync(bool enabled);
        bool IsVSync() const;

    private:
        void Init(const WindowProps& props);
        void Shutdown();

    private:
        GLFWwindow* mWindow;
        GLFWcursor* mMouseCursors[9] = { 0 };

        struct WindowData
        {
            std::string Title;
            unsigned int Width, Height;
            bool VSync;

            EventCallbackFn EventCallback;
        };
        WindowData mData;
    };
}

