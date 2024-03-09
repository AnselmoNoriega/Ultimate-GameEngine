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
        float TexIndex;
    };

    struct Renderer2DStorage
    {
        static const uint32_t MaxQuads = 1000;
        static const uint32_t MaxVertices = MaxQuads * 4;
        static const uint32_t MaxIndices = MaxQuads * 6;
        static const uint32_t MaxTextureSlots = 32;

        Ref<VertexArray> VertexArray;
        Ref<VertexBuffer> VertexBuffer;
        Ref<Shader> ObjShader;
        Ref<Texture2D> EmptyTexture;

        QuadVertex* VertexBufferBase = nullptr;
        QuadVertex* VertexBufferPtr = nullptr;
        uint32_t IndexCount = 0;

        std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
        uint32_t TextureSlotIndex = 1;

        glm::vec4 VertexPositions[4];
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
                {ShaderDataType::Float4, "aColor"},
                {ShaderDataType::Float, "aTexIndex"}
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

        int32_t samplers[sData.MaxTextureSlots];
        for (uint32_t i = 0; i < sData.MaxTextureSlots; ++i)
        {
            samplers[i] = i;
        }

        sData.ObjShader = Shader::Create("Assets/Shaders/Texture");
        sData.ObjShader->Bind();
        sData.ObjShader->SetIntArray("uTextures", samplers, sData.MaxTextureSlots);

        sData.TextureSlots[0] = sData.EmptyTexture;

        sData.VertexPositions[0] = { -0.5, -0.5, 0.0, 1.0f };
        sData.VertexPositions[1] = {  0.5, -0.5, 0.0, 1.0f };
        sData.VertexPositions[2] = {  0.5,  0.5, 0.0, 1.0f };
        sData.VertexPositions[3] = { -0.5,  0.5, 0.0, 1.0f };
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

        sData.TextureSlotIndex = 1;
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
        for (uint32_t i = 0; i < sData.TextureSlotIndex; ++i)
        {
            sData.TextureSlots[i]->Bind(i);
        }
        RenderCommand::DrawIndexed(sData.VertexArray, sData.IndexCount);
    }

    void Renderer2D::FlushAndReset()
    {
        EndScene();
        sData.IndexCount = 0;
        sData.VertexBufferPtr = sData.VertexBufferBase;

        sData.TextureSlotIndex = 1;
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
    {
        DrawQuad({ position.x, position.y, 0.0f }, size, color);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
    {
        NR_PROFILE_FUNCTION();

        if (sData.IndexCount >= Renderer2DStorage::MaxIndices)
        {
            FlushAndReset();
        }

        glm::vec2 halfSize{ size.x / 2, size.y / 2 };
        float textureIndex = 0.0f;

        sData.VertexBufferPtr->Position = { position.x - halfSize.x, position.y - halfSize.y, 0.0f };
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 0.0f, 0.0f };
        sData.VertexBufferPtr->TexIndex = textureIndex;
        ++sData.VertexBufferPtr;

        sData.VertexBufferPtr->Position = { position.x + halfSize.x, position.y - halfSize.y, 0.0f };
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 1.0f, 0.0f };
        sData.VertexBufferPtr->TexIndex = textureIndex;
        ++sData.VertexBufferPtr;

        sData.VertexBufferPtr->Position = { position.x + halfSize.x, position.y + halfSize.y, 0.0f };
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 1.0f, 1.0f };
        sData.VertexBufferPtr->TexIndex = textureIndex;
        ++sData.VertexBufferPtr;

        sData.VertexBufferPtr->Position = { position.x - halfSize.x, position.y + halfSize.y, 0.0f };
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 0.0f, 1.0f };
        sData.VertexBufferPtr->TexIndex = textureIndex;
        ++sData.VertexBufferPtr;

        sData.IndexCount += 6;
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture)
    {
        DrawQuad({ position.x, position.y, 0.0f }, size, texture);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture)
    {
        NR_PROFILE_FUNCTION();

        if (sData.IndexCount >= Renderer2DStorage::MaxIndices)
        {
            FlushAndReset();
        }

        constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
        float textureIndex = 0.0f;

        for (uint32_t i = 0; i < sData.TextureSlotIndex; ++i)
        {
            if (*sData.TextureSlots[i].get() == *texture.get())
            {
                textureIndex = (float)i;
                break;
            }
        }

        if (textureIndex == 0.0f)
        {
            textureIndex = (float)sData.TextureSlotIndex;
            sData.TextureSlots[sData.TextureSlotIndex] = texture;
            ++sData.TextureSlotIndex;
        }

        glm::vec2 halfSize{ size.x / 2, size.y / 2 };

        sData.VertexBufferPtr->Position = { position.x - halfSize.x, position.y - halfSize.y, 0.0f };
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 0.0f, 0.0f };
        sData.VertexBufferPtr->TexIndex = textureIndex;
        ++sData.VertexBufferPtr;

        sData.VertexBufferPtr->Position = { position.x + halfSize.x, position.y - halfSize.y, 0.0f };
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 1.0f, 0.0f };
        sData.VertexBufferPtr->TexIndex = textureIndex;
        ++sData.VertexBufferPtr;

        sData.VertexBufferPtr->Position = { position.x + halfSize.x, position.y + halfSize.y, 0.0f };
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 1.0f, 1.0f };
        sData.VertexBufferPtr->TexIndex = textureIndex;
        ++sData.VertexBufferPtr;

        sData.VertexBufferPtr->Position = { position.x - halfSize.x, position.y + halfSize.y, 0.0f };
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 0.0f, 1.0f };
        sData.VertexBufferPtr->TexIndex = textureIndex;
        ++sData.VertexBufferPtr;

        sData.IndexCount += 6;
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
    {
        DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
    {
        NR_PROFILE_FUNCTION();

        if (sData.IndexCount >= Renderer2DStorage::MaxIndices)
        {
            FlushAndReset();
        }

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
        transform = glm::rotate(transform, rotation, glm::vec3(0, 0, 1));
        transform = glm::scale(transform, { size.x, size.y, 1.0f });

        glm::vec2 halfSize{ size.x / 2, size.y / 2 };
        float textureIndex = 0.0f;

        sData.VertexBufferPtr->Position = transform * sData.VertexPositions[0];
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 0.0f, 0.0f };
        sData.VertexBufferPtr->TexIndex = textureIndex;
        ++sData.VertexBufferPtr;

        sData.VertexBufferPtr->Position = transform * sData.VertexPositions[1];
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 1.0f, 0.0f };
        sData.VertexBufferPtr->TexIndex = textureIndex;
        ++sData.VertexBufferPtr;

        sData.VertexBufferPtr->Position = transform * sData.VertexPositions[2];
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 1.0f, 1.0f };
        sData.VertexBufferPtr->TexIndex = textureIndex;
        ++sData.VertexBufferPtr;

        sData.VertexBufferPtr->Position = transform * sData.VertexPositions[3];
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 0.0f, 1.0f };
        sData.VertexBufferPtr->TexIndex = textureIndex;
        ++sData.VertexBufferPtr;
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture)
    {
        DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture)
    {
        NR_PROFILE_FUNCTION();

        if (sData.IndexCount >= Renderer2DStorage::MaxIndices)
        {
            FlushAndReset();
        }

        constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
        float textureIndex = 0.0f;

        for (uint32_t i = 0; i < sData.TextureSlotIndex; ++i)
        {
            if (*sData.TextureSlots[i].get() == *texture.get())
            {
                textureIndex = (float)i;
                break;
            }
        }

        if (textureIndex == 0.0f)
        {
            textureIndex = (float)sData.TextureSlotIndex;
            sData.TextureSlots[sData.TextureSlotIndex] = texture;
            ++sData.TextureSlotIndex;
        }

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
        transform = glm::rotate(transform, rotation, glm::vec3(0, 0, 1));
        transform = glm::scale(transform, { size.x, size.y, 1.0f });

        glm::vec2 halfSize{ size.x / 2, size.y / 2 };

        sData.VertexBufferPtr->Position = transform * sData.VertexPositions[0];
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 0.0f, 0.0f };
        sData.VertexBufferPtr->TexIndex = textureIndex;
        ++sData.VertexBufferPtr;

        sData.VertexBufferPtr->Position = transform * sData.VertexPositions[1];
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 1.0f, 0.0f };
        sData.VertexBufferPtr->TexIndex = textureIndex;
        ++sData.VertexBufferPtr;

        sData.VertexBufferPtr->Position = transform * sData.VertexPositions[2];
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 1.0f, 1.0f };
        sData.VertexBufferPtr->TexIndex = textureIndex;
        ++sData.VertexBufferPtr;

        sData.VertexBufferPtr->Position = transform * sData.VertexPositions[3];
        sData.VertexBufferPtr->Color = color;
        sData.VertexBufferPtr->TexCoord = { 0.0f, 1.0f };
        sData.VertexBufferPtr->TexIndex = textureIndex;
        ++sData.VertexBufferPtr;
    }
}
