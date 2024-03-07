#include "nrpch.h"
#include "Renderer2D.h"

#include <glm/gtc/matrix_transform.hpp>

#include "VertexArray.h"
#include "Shader.h"
#include "Texture.h"

#include "RenderCommand.h"

namespace NR
{
    struct QuadVertex
    {
        glm::vec3 Position;
        glm::vec2 TexCoord;
        glm::vec4 Color;
    };

    struct Renderer2DStorage
    {
        const uint32_t MaxQuads = 10000;
        const uint32_t MaxVertices = MaxQuads * 4;
        const uint32_t MaxIndices = MaxQuads * 6;

        Ref<VertexArray> VertexArray;
        Ref<VertexBuffer> VertexBuffer;
        Ref<Shader> ObjShader;
        Ref<Texture2D> EmptyTexture;

        QuadVertex* VertexBufferBase = nullptr;
        QuadVertex* VertexBufferPtr = nullptr;
        uint32_t IndexCount = 0;
    };

    static Renderer2DStorage sData;

    void Renderer2D::Init()
    {
        NR_PROFILE_FUNCTION();

        sData.VertexArray = VertexArray::Create();

        sData.VertexBuffer = VertexBuffer::Create(sData.MaxVertices * sizeof(QuadVertex));
        sData.VertexBuffer->SetLayout(
            {
                {ShaderDataType::Float3, "aPosition"},
                {ShaderDataType::Float2, "aTexCoord"},
                {ShaderDataType::Float4, "aColor"}
            });
        sData.VertexArray->AddVertexBuffer(sData.VertexBuffer);

        sData.VertexBufferBase = new QuadVertex[sData.MaxVertices];

        uint32_t* indices = new uint32_t[sData.MaxIndices];

        uint32_t offset = 0;
        for (uint32_t i = 0; i < sData.MaxIndices; i += 6)
        {
            indices[i + 0] = offset + 0;
            indices[i + 1] = offset + 1;
            indices[i + 2] = offset + 2;

            indices[i + 3] = offset + 2;
            indices[i + 4] = offset + 3;
            indices[i + 5] = offset + 0;

            offset += 4;
        }

        Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(indices, sData.MaxIndices);
        sData.VertexArray->SetIndexBuffer(indexBuffer);
        delete[] indices;

        sData.EmptyTexture = Texture2D::Create(1, 1);
        uint32_t emptyTextureData = 0xffffffff;
        sData.EmptyTexture->SetData(&emptyTextureData, sizeof(uint32_t));

        sData.ObjShader = Shader::Create("Assets/Shaders/Texture");
    }

    void Renderer2D::Shutdown()
    {
        NR_PROFILE_FUNCTION();
    }

    void Renderer2D::BeginScene(const OrthographicCamera& camera)
    {
        NR_PROFILE_FUNCTION();

        sData.ObjShader->Bind();
        sData.ObjShader->SetMat4("uViewProjection", camera.GetVPMatrix());

        sData.IndexCount = 0;
        sData.VertexBufferPtr = sData.VertexBufferBase;
    }

    void Renderer2D::EndScene()
    {
        NR_PROFILE_FUNCTION();

        uint32_t dataSize = (uint8_t*)sData.VertexBufferPtr - (uint8_t*)sData.VertexBufferBase;
        sData.VertexBuffer->SetData(sData.VertexBufferBase, dataSize);

        Flush();
    }

    void Renderer2D::Flush()
    {
        RenderCommand::DrawIndexed(sData.VertexArray, sData.IndexCount);
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
    {
        DrawQuad({ position.x, position.y, 0.0f }, size, color);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
    {
        NR_PROFILE_FUNCTION();

        glm::vec2 halfSize{ size.x / 2, size.y / 2};

        sData.VertexBufferPtr->Position = { position.x - halfSize.x, position.y - halfSize.y, 0.0f };
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 0.0f, 0.0f };
        ++sData.VertexBufferPtr;

        sData.VertexBufferPtr->Position = { position.x + halfSize.x, position.y - halfSize.y, 0.0f };
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 1.0f, 0.0f };
        ++sData.VertexBufferPtr;

        sData.VertexBufferPtr->Position = { position.x + halfSize.x, position.y + halfSize.y, 0.0f };
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 1.0f, 1.0f };
        ++sData.VertexBufferPtr;

        sData.VertexBufferPtr->Position = { position.x - halfSize.x, position.y + halfSize.y, 0.0f };
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 0.0f, 1.0f };
        ++sData.VertexBufferPtr;

        sData.IndexCount += 6;

        /*glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
        transform = glm::scale(transform, { size.x, size.y, 1.0f });
        sData.ObjShader->SetMat4("uTransform", transform);

        sData.EmptyTexture->Bind();

        sData.VertexArray->Bind();
        RenderCommand::DrawIndexed(sData.VertexArray);*/
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture)
    {
        DrawQuad({ position.x, position.y, 0.0f }, size, texture);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture)
    {
        NR_PROFILE_FUNCTION();

        sData.ObjShader->SetFloat4("uColor", glm::vec4(1.0));

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
        transform = glm::scale(transform, { size.x, size.y, 1.0f });
        sData.ObjShader->SetMat4("uTransform", transform);

        texture->Bind();

        sData.VertexArray->Bind();
        RenderCommand::DrawIndexed(sData.VertexArray);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
    {
        DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
    {
        NR_PROFILE_FUNCTION();

        sData.ObjShader->SetFloat4("uColor", color);

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
        transform = glm::rotate(transform, rotation, glm::vec3(0, 0, 1));
        transform = glm::scale(transform, { size.x, size.y, 1.0f });
        sData.ObjShader->SetMat4("uTransform", transform);

        sData.EmptyTexture->Bind();

        sData.VertexArray->Bind();
        RenderCommand::DrawIndexed(sData.VertexArray);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture)
    {
        DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture)
    {
        NR_PROFILE_FUNCTION();

        sData.ObjShader->SetFloat4("uColor", glm::vec4(1.0));

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
        transform = glm::rotate(transform, rotation, glm::vec3(0, 0, 1));
        transform = glm::scale(transform, { size.x, size.y, 1.0f });
        sData.ObjShader->SetMat4("uTransform", transform);

        texture->Bind();

        sData.VertexArray->Bind();
        RenderCommand::DrawIndexed(sData.VertexArray);
    }
}
