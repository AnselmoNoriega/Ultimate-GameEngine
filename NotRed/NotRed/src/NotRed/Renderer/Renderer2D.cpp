#include "nrpch.h"
#include "Renderer2D.h"

#include <glm/gtc/matrix_transform.hpp>

#include "NotRed/Renderer/Pipeline.h"
#include "NotRed/Renderer/Shader.h"
#include "NotRed/Renderer/Renderer.h"

namespace NR
{
    struct QuadVertex
    {
        glm::vec3 Position;
        glm::vec4 Color;
        glm::vec2 TexCoord;
        float TexIndex;
        float TilingFactor;
    };

    struct LineVertex
    {
        glm::vec3 Position;
        glm::vec4 Color;
    };

    struct Renderer2DData
    {
        static const uint32_t MaxQuads = 20000;
        static const uint32_t MaxVertices = MaxQuads * 4;
        static const uint32_t MaxIndices = MaxQuads * 6;
        static const uint32_t MaxTextureSlots = 32;

        static const uint32_t MaxLines = 10000;
        static const uint32_t MaxLineVertices = MaxLines * 2;
        static const uint32_t MaxLineIndices = MaxLines * 6;

        Ref<Pipeline> QuadPipeline;
        Ref<VertexBuffer> QuadVertexBuffer;
        Ref<IndexBuffer> QuadIndexBuffer;
        Ref<Shader> TextureShader;
        Ref<Texture2D> WhiteTexture;

        uint32_t QuadIndexCount = 0;
        QuadVertex* QuadVertexBufferBase = nullptr;
        QuadVertex* QuadVertexBufferPtr = nullptr;

        std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
        uint32_t TextureSlotIndex = 1; // 0 = white texture

        glm::vec4 QuadVertexPositions[4];

        Ref<Pipeline> LinePipeline;
        Ref<VertexBuffer> LineVertexBuffer;
        Ref<IndexBuffer> LineIndexBuffer;
        Ref<Shader> LineShader;

        uint32_t LineIndexCount = 0;
        LineVertex* LineVertexBufferBase = nullptr;
        LineVertex* LineVertexBufferPtr = nullptr;

        glm::mat4 CameraViewProj;
        bool DepthTest = true;

        Renderer2D::Statistics Stats;
    };

    static Renderer2DData sData;

    void Renderer2D::Init()
    {
        {
            PipelineSpecification pipelineSpecification;
            pipelineSpecification.Layout = {
                { ShaderDataType::Float3, "a_Position" },
                { ShaderDataType::Float4, "a_Color" },
                { ShaderDataType::Float2, "a_TexCoord" },
                { ShaderDataType::Float, "a_TexIndex" },
                { ShaderDataType::Float, "a_TilingFactor" }
            };
            sData.QuadPipeline = Pipeline::Create(pipelineSpecification);

            sData.QuadVertexBuffer = VertexBuffer::Create(sData.MaxVertices * sizeof(QuadVertex));
            sData.QuadVertexBufferBase = new QuadVertex[sData.MaxVertices];

            uint32_t* quadIndices = new uint32_t[sData.MaxIndices];

            uint32_t offset = 0;
            for (uint32_t i = 0; i < sData.MaxIndices; i += 6)
            {
                quadIndices[i + 0] = offset + 0;
                quadIndices[i + 1] = offset + 1;
                quadIndices[i + 2] = offset + 2;

                quadIndices[i + 3] = offset + 2;
                quadIndices[i + 4] = offset + 3;
                quadIndices[i + 5] = offset + 0;

                offset += 4;
            }

            sData.QuadIndexBuffer = IndexBuffer::Create(quadIndices, sData.MaxIndices);
            delete[] quadIndices;
        }

        sData.WhiteTexture = Texture2D::Create(TextureFormat::RGBA, 1, 1);
        uint32_t whiteTextureData = 0xffffffff;
        sData.WhiteTexture->Lock();
        sData.WhiteTexture->GetWriteableBuffer().Write(&whiteTextureData, sizeof(uint32_t));
        sData.WhiteTexture->Unlock();

        sData.TextureShader = Shader::Create("Assets/Shaders/Renderer2D");

        // Set all texture slots to 0
        sData.TextureSlots[0] = sData.WhiteTexture;

        sData.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
        sData.QuadVertexPositions[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
        sData.QuadVertexPositions[2] = { 0.5f,  0.5f, 0.0f, 1.0f };
        sData.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };

        // Lines
        {
            sData.LineShader = Shader::Create("Assets/Shaders/Renderer2D_Line");

            PipelineSpecification pipelineSpecification;
            pipelineSpecification.Layout = {
                { ShaderDataType::Float3, "a_Position" },
                { ShaderDataType::Float4, "a_Color" }
            };
            sData.LinePipeline = Pipeline::Create(pipelineSpecification);

            sData.LineVertexBuffer = VertexBuffer::Create(sData.MaxLineVertices * sizeof(LineVertex));
            sData.LineVertexBufferBase = new LineVertex[sData.MaxLineVertices];

            uint32_t* lineIndices = new uint32_t[sData.MaxLineIndices];
            for (uint32_t i = 0; i < sData.MaxLineIndices; ++i)
            {
                lineIndices[i] = i;
            }

            sData.LineIndexBuffer = IndexBuffer::Create(lineIndices, sData.MaxLineIndices);
            delete[] lineIndices;
        }
    }

    void Renderer2D::Shutdown()
    {
    }

    void Renderer2D::BeginScene(const glm::mat4& viewProj, bool depthTest)
    {
        sData.CameraViewProj = viewProj;
        sData.DepthTest = depthTest;

        sData.TextureShader->Bind();
        sData.TextureShader->SetMat4("uViewProjection", viewProj);

        sData.QuadIndexCount = 0;
        sData.QuadVertexBufferPtr = sData.QuadVertexBufferBase;

        sData.LineIndexCount = 0;
        sData.LineVertexBufferPtr = sData.LineVertexBufferBase;

        sData.TextureSlotIndex = 1;
    }

    void Renderer2D::EndScene()
    {
        uint32_t dataSize = (uint8_t*)sData.QuadVertexBufferPtr - (uint8_t*)sData.QuadVertexBufferBase;
        if (dataSize)
        {
            sData.QuadVertexBuffer->SetData(sData.QuadVertexBufferBase, dataSize);

            sData.TextureShader->Bind();
            sData.TextureShader->SetMat4("uViewProjection", sData.CameraViewProj);

            for (uint32_t i = 0; i < sData.TextureSlotIndex; ++i)
            {
                sData.TextureSlots[i]->Bind(i);
            }

            sData.QuadPipeline->Bind();
            sData.QuadIndexBuffer->Bind();
            Renderer::DrawIndexed(sData.QuadIndexCount, PrimitiveType::Triangles, sData.DepthTest);
            ++sData.Stats.DrawCalls;
        }

        dataSize = (uint8_t*)sData.LineVertexBufferPtr - (uint8_t*)sData.LineVertexBufferBase;
        if (dataSize)
        {
            sData.LineVertexBuffer->SetData(sData.LineVertexBufferBase, dataSize);

            sData.LineShader->Bind();
            sData.LineShader->SetMat4("uViewProjection", sData.CameraViewProj);

            sData.LinePipeline->Bind();
            sData.LineIndexBuffer->Bind();
            Renderer::SetLineThickness(2.0f);
            Renderer::DrawIndexed(sData.LineIndexCount, PrimitiveType::Lines, sData.DepthTest);
            ++sData.Stats.DrawCalls;
        }

#if OLD
        Flush();
#endif
    }

    void Renderer2D::Flush()
    {
#if OLD
        // Bind textures
        for (uint32_t i = 0; i < sData.TextureSlotIndex; ++i)
            sData.TextureSlots[i]->Bind(i);

        sData.QuadVertexArray->Bind();
        Renderer::DrawIndexed(sData.QuadIndexCount, false);
        sData.Stats.DrawCalls++;
#endif
    }

    void Renderer2D::FlushAndReset()
    {
        EndScene();

        sData.QuadIndexCount = 0;
        sData.QuadVertexBufferPtr = sData.QuadVertexBufferBase;

        sData.TextureSlotIndex = 1;
    }

    void Renderer2D::FlushAndResetLines()
    {

    }

    void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color)
    {
        constexpr size_t quadVertexCount = 4;
        const float textureIndex = 0.0f;
        constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
        const float tilingFactor = 1.0f;

        if (sData.QuadIndexCount >= Renderer2DData::MaxIndices)
        {
            FlushAndReset();
        }

        for (size_t i = 0; i < quadVertexCount; ++i)
        {
            sData.QuadVertexBufferPtr->Position = transform * sData.QuadVertexPositions[i];
            sData.QuadVertexBufferPtr->Color = color;
            sData.QuadVertexBufferPtr->TexCoord = textureCoords[i];
            sData.QuadVertexBufferPtr->TexIndex = textureIndex;
            sData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
            ++sData.QuadVertexBufferPtr;
        }

        sData.QuadIndexCount += 6;

        ++sData.Stats.QuadCount;
    }

    void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        constexpr size_t quadVertexCount = 4;
        constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
        constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

        if (sData.QuadIndexCount >= Renderer2DData::MaxIndices)
        {
            FlushAndReset();
        }

        float textureIndex = 0.0f;
        for (uint32_t i = 1; i < sData.TextureSlotIndex; ++i)
        {
            if (*sData.TextureSlots[i].Raw() == *texture.Raw())
            {
                textureIndex = (float)i;
                break;
            }
        }

        if (textureIndex == 0.0f)
        {
            if (sData.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
            {
                FlushAndReset();
            }

            textureIndex = (float)sData.TextureSlotIndex;
            sData.TextureSlots[sData.TextureSlotIndex] = texture;
            ++sData.TextureSlotIndex;
        }

        for (size_t i = 0; i < quadVertexCount; ++i)
        {
            sData.QuadVertexBufferPtr->Position = transform * sData.QuadVertexPositions[i];
            sData.QuadVertexBufferPtr->Color = color;
            sData.QuadVertexBufferPtr->TexCoord = textureCoords[i];
            sData.QuadVertexBufferPtr->TexIndex = textureIndex;
            sData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
            ++sData.QuadVertexBufferPtr;
        }

        sData.QuadIndexCount += 6;

        ++sData.Stats.QuadCount;
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
    {
        DrawQuad({ position.x, position.y, 0.0f }, size, color);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
    {
        if (sData.QuadIndexCount >= Renderer2DData::MaxIndices)
        {
            FlushAndReset();
        }

        const float textureIndex = 0.0f; // White Texture
        const float tilingFactor = 1.0f;

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

        sData.QuadVertexBufferPtr->Position = transform * sData.QuadVertexPositions[0];
        sData.QuadVertexBufferPtr->Color = color;
        sData.QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
        sData.QuadVertexBufferPtr->TexIndex = textureIndex;
        sData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData.QuadVertexBufferPtr;

        sData.QuadVertexBufferPtr->Position = transform * sData.QuadVertexPositions[1];
        sData.QuadVertexBufferPtr->Color = color;
        sData.QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
        sData.QuadVertexBufferPtr->TexIndex = textureIndex;
        sData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData.QuadVertexBufferPtr;

        sData.QuadVertexBufferPtr->Position = transform * sData.QuadVertexPositions[2];
        sData.QuadVertexBufferPtr->Color = color;
        sData.QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
        sData.QuadVertexBufferPtr->TexIndex = textureIndex;
        sData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData.QuadVertexBufferPtr;

        sData.QuadVertexBufferPtr->Position = transform * sData.QuadVertexPositions[3];
        sData.QuadVertexBufferPtr->Color = color;
        sData.QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
        sData.QuadVertexBufferPtr->TexIndex = textureIndex;
        sData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData.QuadVertexBufferPtr;

        sData.QuadIndexCount += 6;

        ++sData.Stats.QuadCount;
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        DrawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintColor);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        if (sData.QuadIndexCount >= Renderer2DData::MaxIndices)
        {
            FlushAndReset();
        }

        constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

        float textureIndex = 0.0f;
        for (uint32_t i = 1; i < sData.TextureSlotIndex; ++i)
        {
            if (*sData.TextureSlots[i].Raw() == *texture.Raw())
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

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

        sData.QuadVertexBufferPtr->Position = transform * sData.QuadVertexPositions[0];
        sData.QuadVertexBufferPtr->Color = color;
        sData.QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
        sData.QuadVertexBufferPtr->TexIndex = textureIndex;
        sData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData.QuadVertexBufferPtr;

        sData.QuadVertexBufferPtr->Position = transform * sData.QuadVertexPositions[1];
        sData.QuadVertexBufferPtr->Color = color;
        sData.QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
        sData.QuadVertexBufferPtr->TexIndex = textureIndex;
        sData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData.QuadVertexBufferPtr;

        sData.QuadVertexBufferPtr->Position = transform * sData.QuadVertexPositions[2];
        sData.QuadVertexBufferPtr->Color = color;
        sData.QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
        sData.QuadVertexBufferPtr->TexIndex = textureIndex;
        sData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData.QuadVertexBufferPtr;

        sData.QuadVertexBufferPtr->Position = transform * sData.QuadVertexPositions[3];
        sData.QuadVertexBufferPtr->Color = color;
        sData.QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
        sData.QuadVertexBufferPtr->TexIndex = textureIndex;
        sData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData.QuadVertexBufferPtr;

        sData.QuadIndexCount += 6;

        ++sData.Stats.QuadCount;
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
    {
        DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
    {
        if (sData.QuadIndexCount >= Renderer2DData::MaxIndices)
        {
            FlushAndReset();
        }

        const float textureIndex = 0.0f; // White Texture
        const float tilingFactor = 1.0f;

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
            * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

        sData.QuadVertexBufferPtr->Position = transform * sData.QuadVertexPositions[0];
        sData.QuadVertexBufferPtr->Color = color;
        sData.QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
        sData.QuadVertexBufferPtr->TexIndex = textureIndex;
        sData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData.QuadVertexBufferPtr;

        sData.QuadVertexBufferPtr->Position = transform * sData.QuadVertexPositions[1];
        sData.QuadVertexBufferPtr->Color = color;
        sData.QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
        sData.QuadVertexBufferPtr->TexIndex = textureIndex;
        sData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData.QuadVertexBufferPtr;

        sData.QuadVertexBufferPtr->Position = transform * sData.QuadVertexPositions[2];
        sData.QuadVertexBufferPtr->Color = color;
        sData.QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
        sData.QuadVertexBufferPtr->TexIndex = textureIndex;
        sData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData.QuadVertexBufferPtr;

        sData.QuadVertexBufferPtr->Position = transform * sData.QuadVertexPositions[3];
        sData.QuadVertexBufferPtr->Color = color;
        sData.QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
        sData.QuadVertexBufferPtr->TexIndex = textureIndex;
        sData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData.QuadVertexBufferPtr;

        sData.QuadIndexCount += 6;

        ++sData.Stats.QuadCount;
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintColor);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        if (sData.QuadIndexCount >= Renderer2DData::MaxIndices)
        {
            FlushAndReset();
        }

        constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

        float textureIndex = 0.0f;
        for (uint32_t i = 1; i < sData.TextureSlotIndex; ++i)
        {
            if (*sData.TextureSlots[i].Raw() == *texture.Raw())
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

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
            * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

        sData.QuadVertexBufferPtr->Position = transform * sData.QuadVertexPositions[0];
        sData.QuadVertexBufferPtr->Color = color;
        sData.QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
        sData.QuadVertexBufferPtr->TexIndex = textureIndex;
        sData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData.QuadVertexBufferPtr;

        sData.QuadVertexBufferPtr->Position = transform * sData.QuadVertexPositions[1];
        sData.QuadVertexBufferPtr->Color = color;
        sData.QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
        sData.QuadVertexBufferPtr->TexIndex = textureIndex;
        sData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData.QuadVertexBufferPtr;

        sData.QuadVertexBufferPtr->Position = transform * sData.QuadVertexPositions[2];
        sData.QuadVertexBufferPtr->Color = color;
        sData.QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
        sData.QuadVertexBufferPtr->TexIndex = textureIndex;
        sData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData.QuadVertexBufferPtr;

        sData.QuadVertexBufferPtr->Position = transform * sData.QuadVertexPositions[3];
        sData.QuadVertexBufferPtr->Color = color;
        sData.QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
        sData.QuadVertexBufferPtr->TexIndex = textureIndex;
        sData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData.QuadVertexBufferPtr;

        sData.QuadIndexCount += 6;

        ++sData.Stats.QuadCount;
    }

    void Renderer2D::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color)
    {
        if (sData.LineIndexCount >= Renderer2DData::MaxLineIndices)
        {
            FlushAndResetLines();
        }

        sData.LineVertexBufferPtr->Position = p0;
        sData.LineVertexBufferPtr->Color = color;
        ++sData.LineVertexBufferPtr;

        sData.LineVertexBufferPtr->Position = p1;
        sData.LineVertexBufferPtr->Color = color;
        ++sData.LineVertexBufferPtr;

        sData.LineIndexCount += 2;

        ++sData.Stats.LineCount;
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