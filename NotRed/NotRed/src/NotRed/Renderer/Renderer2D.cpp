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
        glm::vec4 Color = { 0.0f, 0.0f, 0.0f, 0.0f };
        float TexIndex = 0.0f;

        int EntityID = -1;
    };

    struct LineVertex
    {
        glm::vec3 Position;
        glm::vec4 Color;
    };

    struct Renderer2DStorage
    {
        static const uint32_t MaxQuads = 20000;
        static const uint32_t MaxVertices = MaxQuads * 4;
        static const uint32_t MaxIndices = MaxQuads * 6;
        static const uint32_t MaxTextureSlots = 32;

        Ref<VertexArray> LineVertexArray;
        Ref<VertexBuffer> LineVertexBuffer;
        Ref<Shader> LineShader;

        Ref<VertexArray> VertexArray;
        Ref<VertexBuffer> VertexBuffer;
        Ref<Shader> ObjShader;
        Ref<Texture2D> EmptyTexture;

        QuadVertex* VertexBufferBase = nullptr;
        QuadVertex* VertexBufferPtr = nullptr;
        uint32_t IndexCount = 0;

        LineVertex* LineVertexBufferBase = nullptr;
        LineVertex* LineVertexBufferPtr = nullptr;
        uint32_t LineVertexCount = 0;

        float LineWidth = 2.0f;

        std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
        uint32_t TextureSlotIndex = 1;

        glm::vec4 VertexPositions[4] = { { -0.5, -0.5, 0.0, 1.0f },
                                         {  0.5, -0.5, 0.0, 1.0f },
                                         {  0.5,  0.5, 0.0, 1.0f },
                                         { -0.5,  0.5, 0.0, 1.0f } };

        glm::vec2 TextureCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f },
                                       { 1.0f, 1.0f }, { 0.0f, 1.0f } };

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

        //Line Set up--------------------------------------
        sData.LineVertexArray = VertexArray::Create();

        sData.LineVertexBuffer = VertexBuffer::Create(sData.MaxVertices * sizeof(LineVertex));
        sData.LineVertexBuffer->SetLayout({
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float4, "a_Color"    }
            });
        sData.LineVertexArray->AddVertexBuffer(sData.LineVertexBuffer);
        sData.LineVertexBufferBase = new LineVertex[sData.MaxVertices];

        sData.LineVertexCount = 0;
        sData.LineVertexBufferPtr = sData.LineVertexBufferBase;
        //Line Set up--------------------------------------

        int32_t samplers[sData.MaxTextureSlots];
        for (uint32_t i = 0; i < sData.MaxTextureSlots; ++i)
        {
            samplers[i] = i;
        }

        sData.ObjShader = Shader::Create("Assets/Shaders/Base");
        sData.LineShader = Shader::Create("assets/shaders/Color");

        sData.TextureSlots[0] = sData.EmptyTexture;

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
        if (sData.IndexCount)
        {
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

        if (sData.LineVertexCount)
        {
            uint32_t dataSize = (uint32_t)((uint8_t*)sData.LineVertexBufferPtr - (uint8_t*)sData.LineVertexBufferBase);
            sData.LineVertexBuffer->SetData(sData.LineVertexBufferBase, dataSize);

            sData.LineShader->Bind();
            RenderCommand::SetLineWidth(sData.LineWidth);
            RenderCommand::DrawLines(sData.LineVertexArray, sData.LineVertexCount);
            sData.Stats.DrawCalls++;
        }
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

    void Renderer2D::DrawQuad(const glm::mat4& transform, float textureIndex, const glm::vec4& color, int entityID)
    {
        NR_PROFILE_FUNCTION();

        size_t quadVertexCount = 4;

        if (sData.IndexCount >= Renderer2DStorage::MaxIndices)
        {
            NextBatch();
        }

        for (size_t i = 0; i < quadVertexCount; i++)
        {
            sData.VertexBufferPtr->Position = transform * sData.VertexPositions[i];
            sData.VertexBufferPtr->TexCoord = sData.TextureCoords[i];
            sData.VertexBufferPtr->Color = color;
            sData.VertexBufferPtr->TexIndex = textureIndex;
            sData.VertexBufferPtr->EntityID = entityID;
            ++sData.VertexBufferPtr;
        }

        sData.IndexCount += 6;

        sData.Stats.QuadCount++;
    }

    float Renderer2D::GetTextureIndex(const Ref<Texture2D>& texture)
    {
        if (!texture)
        {
            return 0.0f;
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

        return textureIndex;
    }

    void Renderer2D::DrawSprite(const glm::mat4& transform, SpriteRendererComponent& src, int entityID)
    {
        DrawQuad(transform, GetTextureIndex(src.Texture), src.Color, entityID);
    }

    void Renderer2D::DrawLine(const glm::vec3& p0, glm::vec3& p1, const glm::vec4& color)
    {
        sData.LineVertexBufferPtr->Position = p0;
        sData.LineVertexBufferPtr->Color = color;
        ++sData.LineVertexBufferPtr;

        sData.LineVertexBufferPtr->Position = p1;
        sData.LineVertexBufferPtr->Color = color;
        sData.LineVertexBufferPtr++;

        sData.LineVertexCount += 2;
    }

    void Renderer2D::DrawRect(const glm::mat4& transform, const glm::vec4& color)
    {
        glm::vec3 lineVertices[4];
        for (size_t i = 0; i < 4; i++)
        {
            lineVertices[i] = transform * sData.VertexPositions[i];
        }

        DrawLine(lineVertices[0], lineVertices[1], color);
        DrawLine(lineVertices[1], lineVertices[2], color);
        DrawLine(lineVertices[2], lineVertices[3], color);
        DrawLine(lineVertices[3], lineVertices[0], color);
    }

    void Renderer2D::SetLineWidth(float width)
    {
        sData.LineWidth = width;
    }

    void Renderer2D::ResetStats()
    {
        memset(&sData.Stats, 0, sizeof(Statistics));
    }

    Renderer2D::Statistics Renderer2D::GetStats()
    {
        return sData.Stats;
    }
}
