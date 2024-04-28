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

#include "Text/MSDFData.h"

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

    struct CircleVertex
    {
        glm::vec3 WorldPosition;
        glm::vec3 LocalPosition;
        glm::vec4 Color;
        float Thickness;

        int EntityID = -1;
    };

    struct LineVertex
    {
        glm::vec3 Position;
        glm::vec4 Color;
    };

    struct TextVertex
    {
        glm::vec3 Position;
        glm::vec4 Color;
        glm::vec2 TexCoord;

        int EntityID;
    };

    struct Renderer2DStorage
    {
        static const uint32_t MaxQuads = 20000;
        static const uint32_t MaxVertices = MaxQuads * 4;
        static const uint32_t MaxIndices = MaxQuads * 6;
        static const uint32_t MaxTextureSlots = 32;

        Ref<VertexArray> QuadVertexArray;
        Ref<VertexBuffer> QuadVertexBuffer;
        Ref<Shader> ObjShader;
        Ref<Texture2D> EmptyTexture;

        Ref<VertexArray> CircleVertexArray;
        Ref<VertexBuffer> CircleVertexBuffer;
        Ref<Shader> CircleShader;

        Ref<VertexArray> LineVertexArray;
        Ref<VertexBuffer> LineVertexBuffer;
        Ref<Shader> LineShader;

        Ref<VertexArray> TextVertexArray;
        Ref<VertexBuffer> TextVertexBuffer;
        Ref<Shader> TextShader;

        uint32_t IndexCount = 0;
        QuadVertex* VertexBufferBase = nullptr;
        QuadVertex* VertexBufferPtr = nullptr;

        uint32_t CircleIndexCount = 0;
        CircleVertex* CircleVertexBufferBase = nullptr;
        CircleVertex* CircleVertexBufferPtr = nullptr;

        uint32_t LineVertexCount = 0;
        LineVertex* LineVertexBufferBase = nullptr;
        LineVertex* LineVertexBufferPtr = nullptr;

        uint32_t TextIndexCount = 0;
        TextVertex* TextVertexBufferBase = nullptr;
        TextVertex* TextVertexBufferPtr = nullptr;
        Ref<Texture2D> FontTextureAtlas;

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

        {
            sData.QuadVertexArray = VertexArray::Create();

            sData.QuadVertexBuffer = VertexBuffer::Create(sData.MaxVertices * sizeof(QuadVertex));
            sData.QuadVertexBuffer->SetLayout(
                {
                    {ShaderDataType::Float3, "aPosition"},
                    {ShaderDataType::Float2, "aTexCoord"},
                    {ShaderDataType::Float4, "aColor"   },
                    {ShaderDataType::Float,  "aTexIndex"},
                    {ShaderDataType::Int,    "aEntityID"}
                });
            sData.QuadVertexArray->AddVertexBuffer(sData.QuadVertexBuffer);
            sData.VertexBufferBase = new QuadVertex[sData.MaxVertices];
        }
        {
            sData.CircleVertexArray = VertexArray::Create();

            sData.CircleVertexBuffer = VertexBuffer::Create(sData.MaxVertices * sizeof(CircleVertex));
            sData.CircleVertexBuffer->SetLayout({
               { ShaderDataType::Float3, "aWorldPosition" },
               { ShaderDataType::Float3, "aLocalPosition" },
               { ShaderDataType::Float4, "aColor"         },
               { ShaderDataType::Float,  "aThickness"     },
               { ShaderDataType::Int,    "aEntityID"      }
                });
            sData.CircleVertexArray->AddVertexBuffer(sData.CircleVertexBuffer);
            sData.CircleVertexBufferBase = new CircleVertex[sData.MaxVertices];
        }
        {
            sData.LineVertexArray = VertexArray::Create();

            sData.LineVertexBuffer = VertexBuffer::Create(sData.MaxVertices * sizeof(LineVertex));
            sData.LineVertexBuffer->SetLayout({
                { ShaderDataType::Float3, "aPosition" },
                { ShaderDataType::Float4, "aColor"    }
                });
            sData.LineVertexArray->AddVertexBuffer(sData.LineVertexBuffer);
            sData.LineVertexBufferBase = new LineVertex[sData.MaxVertices];
        }
        {
            sData.TextVertexArray = VertexArray::Create();

            sData.TextVertexBuffer = VertexBuffer::Create(sData.MaxVertices * sizeof(TextVertex));
            sData.TextVertexBuffer->SetLayout({
                { ShaderDataType::Float3, "aPosition"     },
                { ShaderDataType::Float4, "aColor"        },
                { ShaderDataType::Float2, "aTexCoord"     },
                { ShaderDataType::Int,    "aEntityID"     }
                });
            sData.TextVertexArray->AddVertexBuffer(sData.TextVertexBuffer);
            sData.TextVertexBufferBase = new TextVertex[sData.MaxVertices];
        }
        {
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
            sData.QuadVertexArray->SetIndexBuffer(indexBuffer);
            sData.CircleVertexArray->SetIndexBuffer(indexBuffer);
            sData.TextVertexArray->SetIndexBuffer(indexBuffer);
            delete[] indices;

            sData.EmptyTexture = Texture2D::Create(TextureSpecification());
            uint32_t emptyTextureData = 0xffffffff;
            sData.EmptyTexture->SetData(&emptyTextureData, sizeof(uint32_t));
        }

        sData.ObjShader = Shader::Create("Assets/Shaders/Base");
        sData.LineShader = Shader::Create("assets/shaders/Color");
        sData.CircleShader = Shader::Create("assets/shaders/Circle");
        sData.TextShader = Shader::Create("assets/shaders/Text");

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
            sData.QuadVertexBuffer->SetData(sData.VertexBufferBase, dataSize);

            for (uint32_t i = 0; i < sData.TextureSlotIndex; i++)
            {
                sData.TextureSlots[i]->Bind(i);
            }

            sData.ObjShader->Bind();
            RenderCommand::DrawIndexed(sData.QuadVertexArray, sData.IndexCount);
            ++sData.Stats.DrawCalls;
        }

        if (sData.CircleIndexCount)
        {
            uint32_t dataSize = (uint32_t)((uint8_t*)sData.CircleVertexBufferPtr - (uint8_t*)sData.CircleVertexBufferBase);
            sData.CircleVertexBuffer->SetData(sData.CircleVertexBufferBase, dataSize);

            sData.CircleShader->Bind();
            RenderCommand::DrawIndexed(sData.CircleVertexArray, sData.CircleIndexCount);
            ++sData.Stats.DrawCalls;
        }

        if (sData.LineVertexCount)
        {
            uint32_t dataSize = (uint32_t)((uint8_t*)sData.LineVertexBufferPtr - (uint8_t*)sData.LineVertexBufferBase);
            sData.LineVertexBuffer->SetData(sData.LineVertexBufferBase, dataSize);

            sData.LineShader->Bind();
            RenderCommand::SetLineWidth(sData.LineWidth);
            RenderCommand::DrawLines(sData.LineVertexArray, sData.LineVertexCount);
            ++sData.Stats.DrawCalls;
        }

        if (sData.TextIndexCount)
        {
            uint32_t dataSize = (uint32_t)((uint8_t*)sData.TextVertexBufferPtr - (uint8_t*)sData.TextVertexBufferBase);
            sData.TextVertexBuffer->SetData(sData.TextVertexBufferBase, dataSize);

            sData.FontTextureAtlas->Bind();

            sData.TextShader->Bind();
            RenderCommand::DrawIndexed(sData.TextVertexArray, sData.TextIndexCount);
            ++sData.Stats.DrawCalls;
        }
    }

    void Renderer2D::StartBatch()
    {
        sData.Stats.QuadCount = 0;

        sData.IndexCount = 0;
        sData.VertexBufferPtr = sData.VertexBufferBase;

        sData.CircleIndexCount = 0;
        sData.CircleVertexBufferPtr = sData.CircleVertexBufferBase;

        sData.LineVertexCount = 0;
        sData.LineVertexBufferPtr = sData.LineVertexBufferBase;

        sData.TextIndexCount = 0;
        sData.TextVertexBufferPtr = sData.TextVertexBufferBase;

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

        ++sData.Stats.QuadCount;
    }

    void Renderer2D::DrawCircle(const glm::mat4& transform, const glm::vec4& color, float thickness, int entityID)
    {
        NR_PROFILE_FUNCTION();

        if (sData.CircleIndexCount >= Renderer2DStorage::MaxIndices)
        {
            NextBatch();
        }

        for (size_t i = 0; i < 4; i++)
        {
            sData.CircleVertexBufferPtr->WorldPosition = transform * sData.VertexPositions[i];
            sData.CircleVertexBufferPtr->LocalPosition = sData.VertexPositions[i] * 2.0f;
            sData.CircleVertexBufferPtr->Color = color;
            sData.CircleVertexBufferPtr->Thickness = thickness;
            sData.CircleVertexBufferPtr->EntityID = entityID;
            ++sData.CircleVertexBufferPtr;
        }

        sData.CircleIndexCount += 6;

        ++sData.Stats.QuadCount;
    }

    void Renderer2D::DrawLine(const glm::vec3& p0, glm::vec3& p1, const glm::vec4& color)
    {
        sData.LineVertexBufferPtr->Position = p0;
        sData.LineVertexBufferPtr->Color = color;
        ++sData.LineVertexBufferPtr;

        sData.LineVertexBufferPtr->Position = p1;
        sData.LineVertexBufferPtr->Color = color;
        ++sData.LineVertexBufferPtr;

        sData.LineVertexCount += 2;
    }

    void Renderer2D::DrawString(const glm::mat4& transform, Ref<Font> font, const glm::vec4& color, const std::string& string, float lineSpacing, int entityID)
    {
        const auto& fontGeometry = font->GetMSDFData()->FontGeometry;
        const auto& metrics = fontGeometry.getMetrics();
        Ref<Texture2D> fontAtlas = font->GetTextureAtlas();

        sData.FontTextureAtlas = fontAtlas;

        double x = 0.0;
        double y = 0.0;
        double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);

        const float spaceGlyphAdvance = fontGeometry.getGlyph(' ')->getAdvance();

        for (size_t i = 0; i < string.size(); i++)
        {
            char character = string[i];
            if (character == '\r')
            {
                continue;
            }

            if (character == '\n')
            {
                x = 0;
                y -= fsScale * metrics.lineHeight + lineSpacing;
                continue;
            }

            if (character == '\t')
            {
                x += 4.0f * fsScale * spaceGlyphAdvance;
                continue;
            }

            auto glyph = fontGeometry.getGlyph(character);
            if (!glyph)
            {
                glyph = fontGeometry.getGlyph('?');
                if (!glyph)
                {
                    return;
                }
            }

            double left, buttom, right, top;

            glyph->getQuadAtlasBounds(left, buttom, right, top);
            glm::vec2 texCoordMin((float)left, (float)buttom);
            glm::vec2 texCoordMax((float)right, (float)top);

            glyph->getQuadPlaneBounds(left, buttom, right, top);
            glm::vec2 quadMin((float)left, (float)buttom);
            glm::vec2 quadMax((float)right, (float)top);

            quadMin *= fsScale;
            quadMax *= fsScale;
            quadMin += glm::vec2(x, y);
            quadMax += glm::vec2(x, y);

            float texWidth = 1.0f / fontAtlas->GetWidth();
            float texHeight = 1.0f / fontAtlas->GetHeight();
            texCoordMin *= glm::vec2(texWidth, texHeight);
            texCoordMax *= glm::vec2(texWidth, texHeight);

            sData.TextVertexBufferPtr->Position = transform * glm::vec4(quadMin.x, quadMin.y, 0.0f, 1.0f);
            sData.TextVertexBufferPtr->Color = color;
            sData.TextVertexBufferPtr->TexCoord = { texCoordMin.x, texCoordMin.y };
            sData.TextVertexBufferPtr->EntityID = entityID;
            ++sData.TextVertexBufferPtr;

            sData.TextVertexBufferPtr->Position = transform * glm::vec4(quadMax.x, quadMin.y, 0.0f, 1.0f);
            sData.TextVertexBufferPtr->Color = color;
            sData.TextVertexBufferPtr->TexCoord = { texCoordMax.x, texCoordMin.y };
            sData.TextVertexBufferPtr->EntityID = entityID;
            ++sData.TextVertexBufferPtr;

            sData.TextVertexBufferPtr->Position = transform * glm::vec4(quadMax.x, quadMax.y, 0.0f, 1.0f);
            sData.TextVertexBufferPtr->Color = color;
            sData.TextVertexBufferPtr->TexCoord = { texCoordMax.x, texCoordMax.y };
            sData.TextVertexBufferPtr->EntityID = entityID;
            ++sData.TextVertexBufferPtr;

            sData.TextVertexBufferPtr->Position = transform * glm::vec4(quadMin.x, quadMax.y, 0.0f, 1.0f);
            sData.TextVertexBufferPtr->Color = color;
            sData.TextVertexBufferPtr->TexCoord = { texCoordMin.x, texCoordMax.y };
            sData.TextVertexBufferPtr->EntityID = entityID;
            ++sData.TextVertexBufferPtr;

            sData.TextIndexCount += 6;
            ++sData.Stats.QuadCount;

            if (i < string.size() - 1)
            {
                double advance = glyph->getAdvance();
                char nextCharacter = string[i + 1];
                fontGeometry.getAdvance(advance, character, nextCharacter);

                x += fsScale * advance;
            }
        }
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

    float Renderer2D::GetTextureIndex(const Ref<Texture2D>& texture)
    {
        if (!texture)
        {
            return 0.0f;
        }

        float textureIndex = 0.0f;
        for (uint32_t i = 1; i < sData.TextureSlotIndex; ++i)
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
            ++sData.TextureSlotIndex;
        }

        return textureIndex;
    }

    void Renderer2D::DrawSprite(const glm::mat4& transform, SpriteRendererComponent& src, int entityID)
    {
        DrawQuad(transform, GetTextureIndex(src.Texture), src.Color, entityID);
    }

    void Renderer2D::DrawString(const glm::mat4& transform, TextComponent& tc, int entityID)
    {
        DrawString(transform, tc.TextFont, tc.Color, tc.TextString, tc.LineSpacing, entityID);
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
