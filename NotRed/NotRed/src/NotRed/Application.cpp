#include "nrpch.h"
#include "Application.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "NotRed/Log.h"

#include "Input.h"

#include <glad/glad.h>

namespace NR
{
#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

    Application* Application::sInstance = nullptr;

    static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
    {
        switch (type)
        {
        case NR::ShaderDataType::None:    return GL_FLOAT;
        case NR::ShaderDataType::Float:   return GL_FLOAT;
        case NR::ShaderDataType::Float2:  return GL_FLOAT;
        case NR::ShaderDataType::Float3:  return GL_FLOAT;
        case NR::ShaderDataType::Float4:  return GL_FLOAT;
        case NR::ShaderDataType::Mat3:    return GL_FLOAT;
        case NR::ShaderDataType::Mat4:    return GL_FLOAT;
        default:
        {
            NR_CORE_ASSERT(false, "Unknown ShaderDataType!");
            return 0;
        }
        }
    }

    Application::Application()
    {
        NR_CORE_ASSERT(!sInstance, "There can only be one application");
        sInstance = this;

        mWindow = std::unique_ptr<Window>(Window::Create());
        mWindow->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

        mImGuiLayer = new ImGuiLayer();
        PushOverlay(mImGuiLayer);

        glGenVertexArrays(1, &mVertexArray);
        glBindVertexArray(mVertexArray);

        float vertices[] =
        {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.0f,  0.5f, 0.0f,
        };

        mVertexBuffer.reset(VertexBuffer::Create(vertices, sizeof(vertices)));

        {
            BufferLayout layout =
            {
                {ShaderDataType::Float3, "aPosition"}
            };
            mVertexBuffer->SetLayout(layout);
        }

        uint32_t index = 0;
        for (const auto& element : mVertexBuffer->GetLayout())
        {
            glEnableVertexAttribArray(index);
            glVertexAttribPointer(
                index,
                element.GetComponentCount(),
                ShaderDataTypeToOpenGLBaseType(element.Type),
                element.Normalized ? GL_TRUE : GL_FALSE,
                mVertexBuffer->GetLayout().GetStride(),
                (const void*)element.Offset
            );
            ++index;
        }

        uint32_t indices[3] = { 0, 1, 2 };
        mIndexBuffer.reset(IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));

        std::string vertexSrc = R"(
        #version 330 core
        
        layout(location = 0) in vec3 aPosition;
        out vec3 vPosition;
        
        void main()
        {
            vPosition = aPosition;
            gl_Position = vec4(aPosition, 1.0);
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
            glClearColor(0.05f, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);

            mShader->Bind();
            glBindVertexArray(mVertexArray);
            glDrawElements(GL_TRIANGLES, mIndexBuffer->GetCount(), GL_UNSIGNED_INT, nullptr);

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

