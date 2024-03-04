#include "nrpch.h"
#include "Renderer2D.h"

#include <glm/gtc/matrix_transform.hpp>

#include "VertexArray.h"
#include "Shader.h"
#include "Texture.h"

#include "RenderCommand.h"

namespace NR
{
    struct Renderer2DStorage
    {
        Ref<VertexArray> VertArr;
        Ref<Shader> ColorShader;
        Ref<Shader> TextureShader;
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

        sData->ColorShader = Shader::Create("Assets/Shaders/Color");
        sData->TextureShader = Shader::Create("Assets/Shaders/Texture");

        sData->TextureShader->Bind();
        sData->TextureShader->SetInt("uTexture", 0);
    }

    void Renderer2D::Shutdown()
    {
        delete sData;
    }

    void Renderer2D::BeginScene(const OrthographicCamera& camera)
    {
        sData->ColorShader->Bind();
        sData->ColorShader->SetMat4("uViewProjection", camera.GetVPMatrix());

        sData->TextureShader->Bind();
        sData->TextureShader->SetMat4("uViewProjection", camera.GetVPMatrix());
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
        sData->ColorShader->Bind();
        sData->ColorShader->SetFloat4("uColor", color);

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
        //transform = glm::rotate(transform, glm::radians(0.0f), glm::vec3(0, 0, 1));
        transform = glm::scale(transform, { size.x, size.y, 1.0f });
        sData->ColorShader->SetMat4("uTransform", transform);

        sData->VertArr->Bind();
        RenderCommand::DrawIndexed(sData->VertArr);
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture)
    {
        DrawQuad({position.x, position.y, 0.0f}, size, texture);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture)
    {
        sData->TextureShader->Bind();

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
        //transform = glm::rotate(transform, glm::radians(0.0f), glm::vec3(0, 0, 1));
        transform = glm::scale(transform, { size.x, size.y, 1.0f });
        sData->TextureShader->SetMat4("uTransform", transform);

        texture->Bind();

        sData->VertArr->Bind();
        RenderCommand::DrawIndexed(sData->VertArr);
    }
}
