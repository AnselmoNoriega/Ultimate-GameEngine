#pragma once

#include <GLFW/glfw3.h>

#include "NotRed/Core/Window.h"
#include "NotRed/Renderer/RendererContext.h"

namespace NR
{
    class WinWindow : public Window
    {
    public:
        WinWindow(const WindowProps& props);
        ~WinWindow() override;

        void ProcessEvents() override;
        void SwapBuffers() override;

        inline uint32_t GetWidth() const override { return mData.Width; }
        inline uint32_t GetHeight() const override { return mData.Height; }

        void SetEventCallback(const EventCallbackFn& callback) override { mData.EventCallback = callback; }
        void SetVSync(bool enabled) override;
        bool IsVSync() const override;

        void Maximize() override;

        const std::string& GetTitle() const override { return mData.Title; }
        void SetTitle(const std::string& title) override;

        inline void* GetNativeWindow() const override { return mWindow; }
        std::pair<float, float> GetWindowPos() const override;
        std::pair<uint32_t, uint32_t> GetSize() const override { return { mData.Width, mData.Height }; }

        Ref<RendererContext> GetRenderContext() override { return mRendererContext; }

    private:
        void Init(const WindowProps& props);
        void Shutdown();

    private:
        GLFWwindow* mWindow;
        GLFWcursor* mImGuiMouseCursors[9] = { 0 };

        struct WindowData
        {
            std::string Title;
            uint32_t Width, Height;
            bool VSync;

            EventCallbackFn EventCallback;
        };
        WindowData mData;

        float mLastFrameTime = 0.0f;

        Ref<RendererContext> mRendererContext;
    };
}