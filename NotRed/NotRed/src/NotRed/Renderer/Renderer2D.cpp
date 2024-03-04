#include "nrpch.h"
#include "Renderer2D.h"

#include "VertexArray.h"
#include "Shader.h"

#include "RenderCommand.h"

#include "Platform/OpenGL/GLShader.h"

namespace NR
{
    struct Renderer2DStorage
    {
        Ref<VertexArray> VertArr;
        Ref<Shader> FlatColorShader;
    };

    static Renderer2DStorage* sData;

    void Renderer2D::Init()
    {
        sData = new Renderer2DStorage;

        sData->VertArr = VertexArray::Create();

        float vertices[5 * 4] =
        {
            -0.5f, -0.5f, 0.0f,    0.0f, 0.0f,
             0.5f, -0.5f, 0.0f,    1.0f, 0.0f,
             0.5f,  0.5f, 0.0f,    1.0f, 1.0f,
            -0.5f,  0.5f, 0.0f,    0.0f, 1.0f
        };

        Ref<VertexBuffer> vertexBuffer;
        vertexBuffer.reset(VertexBuffer::Create(vertices, sizeof(vertices)));

        BufferLayout layout =
        {
            {ShaderDataType::Float3, "aPosition"},
            {ShaderDataType::Float2, "aTexCoord"}
        };

        vertexBuffer->SetLayout(layout);
        sData->VertArr->AddVertexBuffer(vertexBuffer);

        uint32_t indices[2 * 3] = { 0, 1, 2, 2, 3, 0 };
        Ref<IndexBuffer>indexBuffer;
        indexBuffer.reset(IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));
        sData->VertArr->SetIndexBuffer(indexBuffer);

        sData->FlatColorShader = Shader::Create("Assets/Shaders/Color");
    }

    void Renderer2D::Shutdown()
    {
        delete sData;
    }

    void Renderer2D::BeginScene(const OrthographicCamera& camera)
    {
        std::dynamic_pointer_cast<GLShader>(sData->FlatColorShader)->Bind();
        std::dynamic_pointer_cast<GLShader>(sData->FlatColorShader)->SetUniformMat4("uViewProjection", camera.GetVPMatrix());
        std::dynamic_pointer_cast<GLShader>(sData->FlatColorShader)->SetUniformMat4("uTransform", glm::mat4(1.0));
    }

    void Renderer2D::EndScene()
    {
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
    {
        DrawQuad({ position.x, position.y, 0.0f }, size, color);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
    {
        std::dynamic_pointer_cast<GLShader>(sData->FlatColorShader)->Bind();
        std::dynamic_pointer_cast<GLShader>(sData->FlatColorShader)->SetUniformFloat4("uColor", color);

        sData->VertArr->Bind();
        RenderCommand::DrawIndexed(sData->VertArr);
    }
}
