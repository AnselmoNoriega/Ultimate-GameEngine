#include "nrpch.h"
#include "Renderer2D.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "VertexArray.h"
#include "Shader.h"
#include "Texture.h"
#include "SubTexture.h"
#include "NotRed/Renderer/UniformBuffer.h"
#include "NotRed/Scene/Components.h"

#include "RenderCommand.h"

namespace NR
{
    struct QuadVertex
    {
        glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
        glm::vec2 TexCoord = { 0.0f, 0.0f };
        glm::vec4 Color = { 0.0f, 0.0f, 0.0f, 0.0f};
        float TexIndex = 0.0f;

        int EntityID = -1;
    };

    struct Renderer2DStorage
    {
        static const uint32_t MaxQuads = 20000;
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
        Renderer2D::Statistics Stats;


        struct CameraData
        {
            glm::mat4 ViewProjection;
        };
        CameraData CameraBuffer;
        Ref<UniformBuffer> CameraUniformBuffer;
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
                {ShaderDataType::Float4, "aColor"   },
                {ShaderDataType::Float,  "aTexIndex"},
                {ShaderDataType::Int,    "aEntityID"}
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

        sData.TextureSlots[0] = sData.EmptyTexture;

        sData.VertexPositions[0] = { -0.5, -0.5, 0.0, 1.0f };
        sData.VertexPositions[1] = { 0.5, -0.5, 0.0, 1.0f };
        sData.VertexPositions[2] = { 0.5,  0.5, 0.0, 1.0f };
        sData.VertexPositions[3] = { -0.5,  0.5, 0.0, 1.0f };

        sData.CameraUniformBuffer = UniformBuffer::Create(sizeof(Renderer2DStorage::CameraData), 0);
    }

    void Renderer2D::Shutdown()
    {
        NR_PROFILE_FUNCTION();

        delete[] sData.VertexBufferBase;
    }

    void Renderer2D::BeginScene(const Camera& camera, glm::mat4 transform)
    {
        NR_PROFILE_FUNCTION();

        sData.CameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(transform);
        sData.CameraUniformBuffer->SetData(&sData.CameraBuffer, sizeof(Renderer2DStorage::CameraData));

        StartBatch();
    }

    void Renderer2D::BeginScene(const EditorCamera& camera)
    {
        NR_PROFILE_FUNCTION();
        
		sData.CameraBuffer.ViewProjection = camera.GetViewProjection();
		sData.CameraUniformBuffer->SetData(&sData.CameraBuffer, sizeof(Renderer2DStorage::CameraData));

        StartBatch();
    }

    void Renderer2D::BeginScene(const OrthographicCamera& camera)
    {
        NR_PROFILE_FUNCTION();

        sData.ObjShader->Bind();
        sData.ObjShader->SetMat4("uViewProjection", camera.GetVPMatrix());

        StartBatch();
    }

    void Renderer2D::EndScene()
    {
        NR_PROFILE_FUNCTION();

        Flush();
    }

    void Renderer2D::Flush()
    {
        if (sData.IndexCount == 0)
        {
            return; 
        }

        uint32_t dataSize = (uint32_t)((uint8_t*)sData.VertexBufferPtr - (uint8_t*)sData.VertexBufferBase);
        sData.VertexBuffer->SetData(sData.VertexBufferBase, dataSize);

        for (uint32_t i = 0; i < sData.TextureSlotIndex; i++)
        {
            sData.TextureSlots[i]->Bind(i);
        }

        sData.ObjShader->Bind();
        RenderCommand::DrawIndexed(sData.VertexArray, sData.IndexCount);
        sData.Stats.DrawCalls++;
    }

    void Renderer2D::StartBatch()
    {
        sData.IndexCount = 0;
        sData.Stats.QuadCount = 0;
        sData.VertexBufferPtr = sData.VertexBufferBase;

        sData.TextureSlotIndex = 1;
    }

    void Renderer2D::NextBatch()
    {
        Flush();
        StartBatch();
    }

    void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID)
    {
        NR_PROFILE_FUNCTION();

        size_t quadVertexCount = 4;

        const float textureIndex = 0.0f;
        constexpr glm::vec2 textureCoords[] = { 
            { 0.0f, 0.0f }, { 1.0f, 0.0f }, 
            { 1.0f, 1.0f }, { 0.0f, 1.0f } 
        };

        if (sData.IndexCount >= Renderer2DStorage::MaxIndices)
        {
            NextBatch();
        }

        for (size_t i = 0; i < quadVertexCount; i++)
        {
            sData.VertexBufferPtr->Position = transform * sData.VertexPositions[i];
            sData.VertexBufferPtr->TexCoord = textureCoords[i];
            sData.VertexBufferPtr->Color = color;
            sData.VertexBufferPtr->TexIndex = textureIndex;
            sData.VertexBufferPtr->EntityID = entityID;
            sData.VertexBufferPtr++;
        }

        sData.IndexCount += 6;

        sData.Stats.QuadCount++;
    }

    void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, const glm::vec4& color, int entityID)
    {
        NR_PROFILE_FUNCTION();

        size_t quadVertexCount = 4;

        constexpr glm::vec2 textureCoords[] = {
            { 0.0f, 0.0f }, { 1.0f, 0.0f },
            { 1.0f, 1.0f }, { 0.0f, 1.0f }
        };

        if (sData.IndexCount >= Renderer2DStorage::MaxIndices)
        {
            NextBatch();
        }

        float textureIndex = 0.0f;
        for (uint32_t i = 1; i < sData.TextureSlotIndex; i++)
        {
            if (*sData.TextureSlots[i] == *texture)
            {
                textureIndex = (float)i;
                break;
            }
        }

        if (textureIndex == 0.0f)
        {
            if (sData.TextureSlotIndex >= Renderer2DStorage::MaxTextureSlots)
            {
                NextBatch();
            }

            textureIndex = (float)sData.TextureSlotIndex;
            sData.TextureSlots[sData.TextureSlotIndex] = texture;
            sData.TextureSlotIndex++;
        }

        for (size_t i = 0; i < quadVertexCount; i++)
        {
            sData.VertexBufferPtr->Position = transform * sData.VertexPositions[i];
            sData.VertexBufferPtr->TexCoord = textureCoords[i];
            sData.VertexBufferPtr->Color = color;
            sData.VertexBufferPtr->TexIndex = textureIndex;
            sData.VertexBufferPtr->EntityID = entityID;
            sData.VertexBufferPtr++;
        }

        sData.IndexCount += 6;

        sData.Stats.QuadCount++;
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
    {
        NR_PROFILE_FUNCTION();

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

        DrawQuad(transform, color);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture)
    {
        NR_PROFILE_FUNCTION();

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

        DrawQuad(transform, texture);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
    {
        NR_PROFILE_FUNCTION();

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
        transform = glm::rotate(transform, rotation, glm::vec3(0, 0, 1));
        transform = glm::scale(transform, { size.x, size.y, 1.0f });

        DrawQuad(transform, color);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture)
    {
        NR_PROFILE_FUNCTION();

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
        transform = glm::rotate(transform, rotation, glm::vec3(0, 0, 1));
        transform = glm::scale(transform, { size.x, size.y, 1.0f });

        DrawQuad(transform, texture);
    }

    void Renderer2D::DrawSprite(const glm::mat4& transform, SpriteRendererComponent& src, int entityID)
    {
        DrawQuad(transform, src.Color, entityID);
    }

    void Renderer2D::ResetStats()
    {
        memset(&sData.Stats, 0, sizeof(Statistics));
    }

    Renderer2D::Statistics Renderer2D::GetStats()
    {
        return sData.Stats;
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
    {
        DrawQuad({ position.x, position.y, 0.0f }, size, color);
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture)
    {
        DrawQuad({ position.x, position.y, 0.0f }, size, texture);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
    {
        DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture)
    {
        DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture);
    }
}
