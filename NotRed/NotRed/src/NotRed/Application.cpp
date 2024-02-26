#include "nrpch.h"
#include "Application.h"

#include "NotRed/Log.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

#include "Input.h"

#include "NotRed/Renderer/Renderer.h"

namespace NR
{
#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

    Application* Application::sInstance = nullptr;

    Application::Application()
        : mCamera()
    {
        NR_CORE_ASSERT(!sInstance, "There can only be one application");
        sInstance = this;

        mWindow = std::unique_ptr<Window>(Window::Create());
        mWindow->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

        mImGuiLayer = new ImGuiLayer();
        PushOverlay(mImGuiLayer);

        mVertexArray.reset(VertexArray::Create());

        float vertices[] =
        {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.0f,  0.5f, 0.0f,
        };

        std::shared_ptr<VertexBuffer> vertexBuffer;
        vertexBuffer.reset(VertexBuffer::Create(vertices, sizeof(vertices)));

        BufferLayout layout =
        {
            {ShaderDataType::Float3, "aPosition"}
        };

        vertexBuffer->SetLayout(layout);
        mVertexArray->AddVertexBuffer(vertexBuffer);

        uint32_t indices[3] = { 0, 1, 2 };
        std::shared_ptr<IndexBuffer>indexBuffer;
        indexBuffer.reset(IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));
        mVertexArray->SetIndexBuffer(indexBuffer);

        std::string vertexSrc = R"(
        #version 330 core
        
        layout(location = 0) in vec3 aPosition;
        out vec3 vPosition;

        uniform mat4 uViewProjection;
        
        void main()
        {
            vPosition = aPosition;
            gl_Position = uViewProjection * vec4(aPosition, 1.0);
        }
        )";

        std::string fragmentSrc = R"(
        #version 330 core
        
        out vec4 color;
        in vec3 vPosition;
        
        void main()
        {
            color = vec4(vPosition.xy + 0.3, vPosition.z, 1.0);
        }
        )";

        mShader.reset(new Shader(vertexSrc, fragmentSrc));
    }

    void Application::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));

        for (auto layer = mLayerStack.end(); layer != mLayerStack.begin();)
        {
            (*--layer)->OnEvent(e);
            if (e.Handled)
            {
                break;
            }
        }
    }

    void Application::PushLayer(Layer* layer)
    {
        mLayerStack.PushLayer(layer);
        layer->Attach();
    }

    void Application::PushOverlay(Layer* overlay)
    {
        mLayerStack.PushOverlay(overlay);
        overlay->Attach();
    }

    void Application::Run()
    {
        while (mRunning)
        {
            RenderCommand::SetClearColor({ 0.05f, 0, 0, 1 });

            Renderer::BeginScene(mCamera);

            Renderer::Submit(mShader, mVertexArray);

            Renderer::EndScene();

            for (Layer* layer : mLayerStack)
            {
                layer->Update();
            }

            mImGuiLayer->Begin();
            for (Layer* layer : mLayerStack)
            {
                layer->ImGuiRender();
            }
            mImGuiLayer->End();

            mWindow->Update();
        }
    }

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        mRunning = false;
        return true;
    }

    Application::~Application()
    {
    }
}

