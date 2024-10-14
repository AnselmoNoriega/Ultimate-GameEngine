#pragma once

#include <GLFW/glfw3.h>

#include "NotRed/Core/Window.h"
#include "NotRed/Renderer/RendererContext.h"
#include "NotRed/Platform/Vulkan/VKSwapChain.h"

namespace NR
{
    class WinWindow : public Window
    {
    public:
        WinWindow(const WindowProps& props);
        ~WinWindow() override;

        void Init() override;
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

        std::pair<float, float> GetWindowPos() const override;
        std::pair<uint32_t, uint32_t> GetSize() const override { return { mData.Width, mData.Height }; }

        inline void* GetNativeWindow() const override { return mWindow; }

        Ref<RendererContext> GetRenderContext() override { return mRendererContext; }
        VKSwapChain& GetSwapChain() override;

    private:
        void Shutdown();

    private:
        GLFWwindow* mWindow;
        GLFWcursor* mImGuiMouseCursors[9] = { 0 };

        WindowProps mProperties;

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
        
        VKSwapChain mSwapChain;
    };
}