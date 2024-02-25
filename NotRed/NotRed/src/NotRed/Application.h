#pragma once

#include "Core.h"

#include "Window.h"
#include "NotRed/LayerStack.h"

#include "NotRed/ImGui/ImGuiLayer.h"

#include "NotRed/Renderer/Shader.h"
#include "NotRed/Renderer/VertexBuffer.h"
#include "NotRed/Renderer/IndexBuffer.h"

namespace NR
{
    class WindowCloseEvent;

    class  Application
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
        ImGuiLayer* mImGuiLayer;
        
        LayerStack mLayerStack;

        unsigned int mVertexArray;
        std::unique_ptr<Shader> mShader;
        std::unique_ptr<VertexBuffer> mVertexBuffer;
        std::unique_ptr<IndexBuffer> mIndexBuffer;
    };

    Application* CreateApplication();
}

