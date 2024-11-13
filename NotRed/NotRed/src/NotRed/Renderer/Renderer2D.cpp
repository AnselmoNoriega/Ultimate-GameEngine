#include "nrpch.h"
#include "Renderer2D.h"

#include <glm/gtc/matrix_transform.hpp>

#include "NotRed/Renderer/Pipeline.h"
#include "NotRed/Renderer/Shader.h"
#include "NotRed/Renderer/Renderer.h"
#include "NotRed/Renderer/RenderCommandBuffer.h"

#include "NotRed/Platform/Vulkan/VKRenderCommandBuffer.h"

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

    struct CircleVertex
    {
        glm::vec3 WorldPosition;
        float Thickness;
        glm::vec2 LocalPosition;
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
        Ref<Material> QuadMaterial;

        Ref<Texture2D> WhiteTexture;

        uint32_t QuadIndexCount = 0;
        QuadVertex* QuadVertexBufferBase = nullptr;
        QuadVertex* QuadVertexBufferPtr = nullptr;

        Ref<Shader> CircleShader;
        Ref<Pipeline> CirclePipeline;
        Ref<VertexBuffer> CircleVertexBuffer;

        uint32_t CircleIndexCount = 0;
        CircleVertex* CircleVertexBufferBase = nullptr;
        CircleVertex* CircleVertexBufferPtr = nullptr;

        std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
        uint32_t TextureSlotIndex = 1; // 0 = white texture

        glm::vec4 QuadVertexPositions[4];

        Ref<Pipeline> LinePipeline;
        Ref<Pipeline> LineOnTopPipeline;
        Ref<VertexBuffer> LineVertexBuffer;
        Ref<IndexBuffer> LineIndexBuffer;
        Ref<Material> LineMaterial;

        uint32_t LineIndexCount = 0;
        LineVertex* LineVertexBufferBase = nullptr;
        LineVertex* LineVertexBufferPtr = nullptr;

        glm::mat4 CameraViewProj;
        glm::mat4 CameraView;
        bool DepthTest = true;

        float LineWidth = 1.0f;

        Renderer2D::Statistics Stats;

        Ref<RenderCommandBuffer> CommandBuffer;
        Ref<UniformBufferSet> UniformBufferSet;
        
        struct UBCamera
        {
            glm::mat4 ViewProjection;
        };
    };

    static Renderer2DData* sData = nullptr;

    void Renderer2D::Init()
    {
        sData = new Renderer2DData();

        FrameBufferSpecification frameBufferSpec;
        frameBufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::Depth };
        frameBufferSpec.Samples = 1;
        frameBufferSpec.ClearOnLoad = false;
        frameBufferSpec.ClearColor = { 0.1f, 0.5f, 0.5f, 1.0f };
        frameBufferSpec.DebugName = "Renderer2D FrameBuffer";

        Ref<FrameBuffer> frameBuffer = FrameBuffer::Create(frameBufferSpec);
        RenderPassSpecification renderPassSpec;
        renderPassSpec.TargetFrameBuffer = frameBuffer;
        renderPassSpec.DebugName = "Renderer2D";
        Ref<RenderPass> renderPass = RenderPass::Create(renderPassSpec);

        {
            PipelineSpecification pipelineSpecification;
            pipelineSpecification.DebugName = "Renderer2D-Quad";
            pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Renderer2D");
            pipelineSpecification.RenderPass = renderPass;
            pipelineSpecification.BackfaceCulling = false;

            pipelineSpecification.Layout = {
                { ShaderDataType::Float3, "aPosition" },
                { ShaderDataType::Float4, "aColor" },
                { ShaderDataType::Float2, "aTexCoord" },
                { ShaderDataType::Float, "aTexIndex" },
                { ShaderDataType::Float, "aTilingFactor" }
            };
            sData->QuadPipeline = Pipeline::Create(pipelineSpecification);

            sData->QuadVertexBuffer = VertexBuffer::Create(sData->MaxVertices * sizeof(QuadVertex));
            sData->QuadVertexBufferBase = new QuadVertex[sData->MaxVertices];

            uint32_t* quadIndices = new uint32_t[sData->MaxIndices];

            uint32_t offset = 0;
            for (uint32_t i = 0; i < sData->MaxIndices; i += 6)
            {
                quadIndices[i + 0] = offset + 0;
                quadIndices[i + 1] = offset + 1;
                quadIndices[i + 2] = offset + 2;

                quadIndices[i + 3] = offset + 2;
                quadIndices[i + 4] = offset + 3;
                quadIndices[i + 5] = offset + 0;

                offset += 4;
            }

            sData->QuadIndexBuffer = IndexBuffer::Create(quadIndices, sData->MaxIndices);
            delete[] quadIndices;
        }

        sData->WhiteTexture = Renderer::GetWhiteTexture();

        // Set all texture slots to 0
        sData->TextureSlots[0] = sData->WhiteTexture;

        sData->QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
        sData->QuadVertexPositions[1] = { -0.5f,  0.5f, 0.0f, 1.0f };
        sData->QuadVertexPositions[2] = { 0.5f,  0.5f, 0.0f, 1.0f };
        sData->QuadVertexPositions[3] = { 0.5f, -0.5f, 0.0f, 1.0f };

        // Lines
        {
            PipelineSpecification pipelineSpecification;
            pipelineSpecification.DebugName = "Renderer2D-Line";
            pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Renderer2D_Line");
            pipelineSpecification.RenderPass = renderPass;
            pipelineSpecification.Topology = PrimitiveTopology::Lines;
            pipelineSpecification.LineWidth = 2.0f;
            pipelineSpecification.Layout = {
                { ShaderDataType::Float3, "aPosition" },
                { ShaderDataType::Float4, "aColor" }
            };
            sData->LinePipeline = Pipeline::Create(pipelineSpecification);
            pipelineSpecification.DepthTest = false;
            sData->LineOnTopPipeline = Pipeline::Create(pipelineSpecification);

            sData->LineVertexBuffer = VertexBuffer::Create(sData->MaxLineVertices * sizeof(LineVertex));
            sData->LineVertexBufferBase = new LineVertex[sData->MaxLineVertices];

            uint32_t* lineIndices = new uint32_t[sData->MaxLineIndices];
            for (uint32_t i = 0; i < sData->MaxLineIndices; ++i)
            {
                lineIndices[i] = i;
            }

            sData->LineIndexBuffer = IndexBuffer::Create(lineIndices, sData->MaxLineIndices);
            delete[] lineIndices;
        }

        // Circles
        {
            PipelineSpecification pipelineSpecification;
            pipelineSpecification.DebugName = "Renderer2D-Circle";
            pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Renderer2D_Circle");
            pipelineSpecification.RenderPass = renderPass;
            pipelineSpecification.Layout = {
                { ShaderDataType::Float3,  "aWorldPosition" },
                { ShaderDataType::Float,   "aThickness" },
                { ShaderDataType::Float2,  "aLocalPosition" },
                { ShaderDataType::Float4,  "aColor" }
            };
            sData->CirclePipeline = Pipeline::Create(pipelineSpecification);

            sData->CircleVertexBuffer = VertexBuffer::Create(sData->MaxVertices * sizeof(QuadVertex));
            sData->CircleVertexBufferBase = new CircleVertex[sData->MaxVertices];
        }

        sData->CommandBuffer = RenderCommandBuffer::Create(0, "Renderer2D");
        uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
        
        sData->UniformBufferSet = UniformBufferSet::Create(framesInFlight);
        sData->UniformBufferSet->Create(sizeof(Renderer2DData::UBCamera), 0);
        sData->QuadMaterial = Material::Create(sData->QuadPipeline->GetSpecification().Shader, "QuadMaterial");
        sData->LineMaterial = Material::Create(sData->LinePipeline->GetSpecification().Shader, "LineMaterial");
    }

    void Renderer2D::Shutdown()
    {
        delete sData;
    }

    void Renderer2D::BeginScene(const glm::mat4& viewProj, const glm::mat4& view, bool depthTest)
    {
        sData->CameraViewProj = viewProj;
        sData->CameraView = view;
        sData->DepthTest = depthTest;

        Renderer::Submit([viewProj]() mutable
            {
                uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
                sData->UniformBufferSet->Get(0, 0, bufferIndex)->RT_SetData(&viewProj, sizeof(Renderer2DData::UBCamera));
            });

        sData->QuadIndexCount = 0;
        sData->QuadVertexBufferPtr = sData->QuadVertexBufferBase;

        sData->LineIndexCount = 0;
        sData->LineVertexBufferPtr = sData->LineVertexBufferBase;

        sData->CircleIndexCount = 0;
        sData->CircleVertexBufferPtr = sData->CircleVertexBufferBase;

        sData->TextureSlotIndex = 1;

        for (uint32_t i = 1; i < sData->TextureSlots.size(); ++i)
        {
            sData->TextureSlots[i] = nullptr;
        }
    }

    void Renderer2D::EndScene()
    {
        sData->CommandBuffer->Begin();
        Renderer::BeginRenderPass(sData->CommandBuffer, sData->QuadPipeline->GetSpecification().RenderPass);

        uint32_t dataSize = (uint32_t)((uint8_t*)sData->QuadVertexBufferPtr - (uint8_t*)sData->QuadVertexBufferBase);
        if (dataSize)
        {
            sData->QuadVertexBuffer->SetData(sData->QuadVertexBufferBase, dataSize);

            for (uint32_t i = 0; i < sData->TextureSlots.size(); ++i)
            {
                if (sData->TextureSlots[i])
                {
                    sData->QuadMaterial->Set("uTextures", sData->TextureSlots[i], i);
                }
                else
                {
                    sData->QuadMaterial->Set("uTextures", sData->WhiteTexture, i);
                }
            }

            Renderer::RenderGeometry(sData->CommandBuffer, sData->QuadPipeline, sData->UniformBufferSet, nullptr, sData->QuadMaterial, sData->QuadVertexBuffer, sData->QuadIndexBuffer, glm::mat4(1.0f), sData->QuadIndexCount);

            ++sData->Stats.DrawCalls;
        }

        dataSize = (uint32_t)((uint8_t*)sData->LineVertexBufferPtr - (uint8_t*)sData->LineVertexBufferBase);
        if (dataSize)
        {
            sData->LineVertexBuffer->SetData(sData->LineVertexBufferBase, dataSize);

            Ref<RenderCommandBuffer> renderCommandBuffer = sData->CommandBuffer;
            Renderer::Submit([renderCommandBuffer]()
                {
                    uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
                    VkCommandBuffer commandBuffer = renderCommandBuffer.As<VKRenderCommandBuffer>()->GetCommandBuffer(frameIndex);
                    vkCmdSetLineWidth(commandBuffer, sData->LineWidth);
                });

            Renderer::RenderGeometry(sData->CommandBuffer, sData->LinePipeline, sData->UniformBufferSet, nullptr, sData->LineMaterial, sData->LineVertexBuffer, sData->LineIndexBuffer, glm::mat4(1.0f), sData->LineIndexCount);

            ++sData->Stats.DrawCalls;
        }

        Renderer::EndRenderPass(sData->CommandBuffer);
        sData->CommandBuffer->End();
        sData->CommandBuffer->Submit();
    }

    void Renderer2D::Flush()
    {
    }

    Ref<RenderPass> Renderer2D::GetTargetRenderPass()
    {
        return sData->QuadPipeline->GetSpecification().RenderPass;
    }

    void Renderer2D::SetTargetRenderPass(Ref<RenderPass> renderPass)
    {
        if (renderPass != sData->QuadPipeline->GetSpecification().RenderPass)
        {
            {
                PipelineSpecification pipelineSpecification = sData->QuadPipeline->GetSpecification();
                pipelineSpecification.RenderPass = renderPass;
                sData->QuadPipeline = Pipeline::Create(pipelineSpecification);
            }
            {
                PipelineSpecification pipelineSpecification = sData->LinePipeline->GetSpecification();
                pipelineSpecification.RenderPass = renderPass;
                sData->LinePipeline = Pipeline::Create(pipelineSpecification);
            }
        }
    }

    void Renderer2D::FlushAndReset()
    {
        EndScene();

        sData->QuadIndexCount = 0;
        sData->QuadVertexBufferPtr = sData->QuadVertexBufferBase;

        sData->TextureSlotIndex = 1;
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

        if (sData->QuadIndexCount >= Renderer2DData::MaxIndices)
        {
            FlushAndReset();
        }

        for (size_t i = 0; i < quadVertexCount; ++i)
        {
            sData->QuadVertexBufferPtr->Position = transform * sData->QuadVertexPositions[i];
            sData->QuadVertexBufferPtr->Color = color;
            sData->QuadVertexBufferPtr->TexCoord = textureCoords[i];
            sData->QuadVertexBufferPtr->TexIndex = textureIndex;
            sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
            ++sData->QuadVertexBufferPtr;
        }

        sData->QuadIndexCount += 6;

        ++sData->Stats.QuadCount;
    }

    void Renderer2D::DrawQuadBillboard(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
    {
        if (sData->QuadIndexCount >= Renderer2DData::MaxIndices)
        {
            FlushAndReset();
        }

        const float textureIndex = 0.0f; // White Texture
        const float tilingFactor = 1.0f;

        glm::vec3 camRightWS = { sData->CameraView[0][0], sData->CameraView[1][0], sData->CameraView[2][0] };
        glm::vec3 camUpWS = { sData->CameraView[0][1], sData->CameraView[1][1], sData->CameraView[2][1] };
        
        sData->QuadVertexBufferPtr->Position = position + camRightWS * (sData->QuadVertexPositions[0].x) * size.x + camUpWS * sData->QuadVertexPositions[0].y * size.y;
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadVertexBufferPtr->Position = position + camRightWS * sData->QuadVertexPositions[1].x * size.x + camUpWS * sData->QuadVertexPositions[1].y * size.y;
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadVertexBufferPtr->Position = position + camRightWS * sData->QuadVertexPositions[2].x * size.x + camUpWS * sData->QuadVertexPositions[2].y * size.y;
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadVertexBufferPtr->Position = position + camRightWS * sData->QuadVertexPositions[3].x * size.x + camUpWS * sData->QuadVertexPositions[3].y * size.y;
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadIndexCount += 6;
        ++sData->Stats.QuadCount;
    }

    void Renderer2D::DrawQuadBillboard(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        if (sData->QuadIndexCount >= Renderer2DData::MaxIndices)
        {
            FlushAndReset();
        }

        constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
        float textureIndex = 0.0f;

        for (uint32_t i = 1; i < sData->TextureSlotIndex; ++i)
        {
            if (*sData->TextureSlots[i].Raw() == *texture.Raw())
            {
                textureIndex = (float)i;
                break;
            }
        }

        if (textureIndex == 0.0f)
        {
            textureIndex = (float)sData->TextureSlotIndex;
            sData->TextureSlots[sData->TextureSlotIndex] = texture;
            ++sData->TextureSlotIndex;
        }

        glm::vec3 camRightWS = { sData->CameraView[0][0], sData->CameraView[1][0], sData->CameraView[2][0] };
        glm::vec3 camUpWS = { sData->CameraView[0][1], sData->CameraView[1][1], sData->CameraView[2][1] };
        
        sData->QuadVertexBufferPtr->Position = position + camRightWS * (sData->QuadVertexPositions[0].x) * size.x + camUpWS * sData->QuadVertexPositions[0].y * size.y;
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadVertexBufferPtr->Position = position + camRightWS * sData->QuadVertexPositions[1].x * size.x + camUpWS * sData->QuadVertexPositions[1].y * size.y;
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadVertexBufferPtr->Position = position + camRightWS * sData->QuadVertexPositions[2].x * size.x + camUpWS * sData->QuadVertexPositions[2].y * size.y;
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadVertexBufferPtr->Position = position + camRightWS * sData->QuadVertexPositions[3].x * size.x + camUpWS * sData->QuadVertexPositions[3].y * size.y;
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadIndexCount += 6;
        ++sData->Stats.QuadCount;
    }

    void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        constexpr size_t quadVertexCount = 4;
        constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
        constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

        if (sData->QuadIndexCount >= Renderer2DData::MaxIndices)
        {
            FlushAndReset();
        }

        float textureIndex = 0.0f;
        for (uint32_t i = 1; i < sData->TextureSlotIndex; ++i)
        {
            if (*sData->TextureSlots[i].Raw() == *texture.Raw())
            {
                textureIndex = (float)i;
                break;
            }
        }

        if (textureIndex == 0.0f)
        {
            if (sData->TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
            {
                FlushAndReset();
            }

            textureIndex = (float)sData->TextureSlotIndex;
            sData->TextureSlots[sData->TextureSlotIndex] = texture;
            ++sData->TextureSlotIndex;
        }

        for (size_t i = 0; i < quadVertexCount; ++i)
        {
            sData->QuadVertexBufferPtr->Position = transform * sData->QuadVertexPositions[i];
            sData->QuadVertexBufferPtr->Color = color;
            sData->QuadVertexBufferPtr->TexCoord = textureCoords[i];
            sData->QuadVertexBufferPtr->TexIndex = textureIndex;
            sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
            ++sData->QuadVertexBufferPtr;
        }

        sData->QuadIndexCount += 6;

        ++sData->Stats.QuadCount;
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
    {
        DrawQuad({ position.x, position.y, 0.0f }, size, color);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
    {
        if (sData->QuadIndexCount >= Renderer2DData::MaxIndices)
        {
            FlushAndReset();
        }

        const float textureIndex = 0.0f; // White Texture
        const float tilingFactor = 1.0f;

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

        sData->QuadVertexBufferPtr->Position = transform * sData->QuadVertexPositions[0];
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadVertexBufferPtr->Position = transform * sData->QuadVertexPositions[1];
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadVertexBufferPtr->Position = transform * sData->QuadVertexPositions[2];
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadVertexBufferPtr->Position = transform * sData->QuadVertexPositions[3];
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadIndexCount += 6;

        ++sData->Stats.QuadCount;
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        DrawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintColor);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        if (sData->QuadIndexCount >= Renderer2DData::MaxIndices)
        {
            FlushAndReset();
        }

        constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

        float textureIndex = 0.0f;
        for (uint32_t i = 1; i < sData->TextureSlotIndex; ++i)
        {
            if (*sData->TextureSlots[i].Raw() == *texture.Raw())
            {
                textureIndex = (float)i;
                break;
            }
        }

        if (textureIndex == 0.0f)
        {
            textureIndex = (float)sData->TextureSlotIndex;
            sData->TextureSlots[sData->TextureSlotIndex] = texture;
            ++sData->TextureSlotIndex;
        }

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

        sData->QuadVertexBufferPtr->Position = transform * sData->QuadVertexPositions[0];
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadVertexBufferPtr->Position = transform * sData->QuadVertexPositions[1];
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadVertexBufferPtr->Position = transform * sData->QuadVertexPositions[2];
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadVertexBufferPtr->Position = transform * sData->QuadVertexPositions[3];
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadIndexCount += 6;

        ++sData->Stats.QuadCount;
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
    {
        DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
    {
        if (sData->QuadIndexCount >= Renderer2DData::MaxIndices)
        {
            FlushAndReset();
        }

        const float textureIndex = 0.0f; // White Texture
        const float tilingFactor = 1.0f;

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
            * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

        sData->QuadVertexBufferPtr->Position = transform * sData->QuadVertexPositions[0];
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadVertexBufferPtr->Position = transform * sData->QuadVertexPositions[1];
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadVertexBufferPtr->Position = transform * sData->QuadVertexPositions[2];
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadVertexBufferPtr->Position = transform * sData->QuadVertexPositions[3];
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadIndexCount += 6;

        ++sData->Stats.QuadCount;
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintColor);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        if (sData->QuadIndexCount >= Renderer2DData::MaxIndices)
        {
            FlushAndReset();
        }

        constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

        float textureIndex = 0.0f;
        for (uint32_t i = 1; i < sData->TextureSlotIndex; ++i)
        {
            if (*sData->TextureSlots[i].Raw() == *texture.Raw())
            {
                textureIndex = (float)i;
                break;
            }
        }

        if (textureIndex == 0.0f)
        {
            textureIndex = (float)sData->TextureSlotIndex;
            sData->TextureSlots[sData->TextureSlotIndex] = texture;
            ++sData->TextureSlotIndex;
        }

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
            * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

        sData->QuadVertexBufferPtr->Position = transform * sData->QuadVertexPositions[0];
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadVertexBufferPtr->Position = transform * sData->QuadVertexPositions[1];
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadVertexBufferPtr->Position = transform * sData->QuadVertexPositions[2];
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadVertexBufferPtr->Position = transform * sData->QuadVertexPositions[3];
        sData->QuadVertexBufferPtr->Color = color;
        sData->QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
        sData->QuadVertexBufferPtr->TexIndex = textureIndex;
        sData->QuadVertexBufferPtr->TilingFactor = tilingFactor;
        ++sData->QuadVertexBufferPtr;

        sData->QuadIndexCount += 6;

        ++sData->Stats.QuadCount;
    }

    void Renderer2D::DrawRotatedRect(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
    {
        DrawRotatedRect({ position.x, position.y, 0.0f }, size, rotation, color);
    }

    void Renderer2D::DrawRotatedRect(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
    {
        if (sData->LineIndexCount >= Renderer2DData::MaxLineIndices)
        {
            FlushAndResetLines();
        }

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
            * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

        glm::vec3 positions[4] =
        {
            transform * sData->QuadVertexPositions[0],
            transform * sData->QuadVertexPositions[1],
            transform * sData->QuadVertexPositions[2],
            transform * sData->QuadVertexPositions[3]
        };

        for (int i = 0; i < 4; ++i)
        {
            auto& v0 = positions[i];
            auto& v1 = positions[(i + 1) % 4];

            sData->LineVertexBufferPtr->Position = v0;
            sData->LineVertexBufferPtr->Color = color;
            ++sData->LineVertexBufferPtr;

            sData->LineVertexBufferPtr->Position = v1;
            sData->LineVertexBufferPtr->Color = color;
            ++sData->LineVertexBufferPtr;

            sData->LineIndexCount += 2;
            ++sData->Stats.LineCount;
        }
    }

    void Renderer2D::FillCircle(const glm::vec2& position, float radius, const glm::vec4& color, float thickness)
    {
        FillCircle({ position.x, position.y, 0.0f }, radius, color, thickness);
    }

    void Renderer2D::FillCircle(const glm::vec3& position, float radius, const glm::vec4& color, float thickness)
    {
        if (sData->CircleIndexCount >= Renderer2DData::MaxIndices)
        {
            FlushAndReset();
        }

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::scale(glm::mat4(1.0f), { radius * 2.0f, radius * 2.0f, 1.0f });

        for (int i = 0; i < 4; ++i)
        {
            sData->CircleVertexBufferPtr->WorldPosition = transform * sData->QuadVertexPositions[i];
            sData->CircleVertexBufferPtr->Thickness = thickness;
            sData->CircleVertexBufferPtr->LocalPosition = sData->QuadVertexPositions[i] * 2.0f;
            sData->CircleVertexBufferPtr->Color = color;
            ++sData->CircleVertexBufferPtr;

            sData->CircleIndexCount += 6;
            ++sData->Stats.QuadCount;
        }

    }

    void Renderer2D::DrawCircle(const glm::vec3& position, const glm::vec3& rotation, float radius, const glm::vec4& color)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::rotate(glm::mat4(1.0f), rotation.x, { 1.0f, 0.0f, 0.0f })
            * glm::rotate(glm::mat4(1.0f), rotation.y, { 0.0f, 1.0f, 0.0f })
            * glm::rotate(glm::mat4(1.0f), rotation.z, { 0.0f, 0.0f, 1.0f })
            * glm::scale(glm::mat4(1.0f), glm::vec3(radius));

        int segments = 32;
        for (int i = 0; i < segments; ++i)
        {
            float angle = 2.0f * glm::pi<float>() * (float)i / segments;
            glm::vec4 startPosition = { glm::cos(angle), glm::sin(angle), 0.0f, 1.0f };
            angle = 2.0f * glm::pi<float>() * (float)((i + 1) % segments) / segments;
            
            glm::vec4 endPosition = { glm::cos(angle), glm::sin(angle), 0.0f, 1.0f };
            glm::vec3 p0 = transform * startPosition;
            glm::vec3 p1 = transform * endPosition;
            
            DrawLine(p0, p1, color);
        }
    }

    void Renderer2D::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color)
    {
        if (sData->LineIndexCount >= Renderer2DData::MaxLineIndices)
        {
            FlushAndResetLines();
        }

        sData->LineVertexBufferPtr->Position = p0;
        sData->LineVertexBufferPtr->Color = color;
        ++sData->LineVertexBufferPtr;

        sData->LineVertexBufferPtr->Position = p1;
        sData->LineVertexBufferPtr->Color = color;
        ++sData->LineVertexBufferPtr;

        sData->LineIndexCount += 2;

        ++sData->Stats.LineCount;
    }

    void Renderer2D::ResetStats()
    {
        memset(&sData->Stats, 0, sizeof(Statistics));
    }

    Renderer2D::Statistics Renderer2D::GetStats()
    {
        return sData->Stats;
    }

    void Renderer2D::DrawAABB(Ref<Mesh> mesh, const glm::mat4& transform, const glm::vec4& color)
    {
        const auto& meshAssetSubmeshes = mesh->GetMeshAsset()->GetSubmeshes();
        auto& submeshes = mesh->GetSubmeshes();

        for (uint32_t submeshIndex : submeshes)
        {
            const Submesh& submesh = meshAssetSubmeshes[submeshIndex];
            auto& aabb = submesh.BoundingBox;
            auto aabbTransform = transform * submesh.Transform;
            DrawAABB(aabb, aabbTransform);
        }
    }

    void Renderer2D::DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color)
    {
        glm::vec4 min = { aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f };
        glm::vec4 max = { aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f };
        glm::vec4 corners[8] =
        {
            transform * glm::vec4 { aabb.Min.x, aabb.Min.y, aabb.Max.z, 1.0f },
            transform * glm::vec4 { aabb.Min.x, aabb.Max.y, aabb.Max.z, 1.0f },
            transform * glm::vec4 { aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f },
            transform * glm::vec4 { aabb.Max.x, aabb.Min.y, aabb.Max.z, 1.0f },
            transform * glm::vec4 { aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f },
            transform * glm::vec4 { aabb.Min.x, aabb.Max.y, aabb.Min.z, 1.0f },
            transform * glm::vec4 { aabb.Max.x, aabb.Max.y, aabb.Min.z, 1.0f },
            transform * glm::vec4 { aabb.Max.x, aabb.Min.y, aabb.Min.z, 1.0f }
        };

        for (uint32_t i = 0; i < 4; ++i)
        {
            DrawLine(corners[i], corners[(i + 1) % 4], color);
        }

        for (uint32_t i = 0; i < 4; ++i)
        {
            DrawLine(corners[i + 4], corners[((i + 1) % 4) + 4], color);
        }

        for (uint32_t i = 0; i < 4; ++i)
        {
            DrawLine(corners[i], corners[i + 4], color);
        }
    }
    
    void Renderer2D::SetLineWidth(float lineWidth)
    {
        sData->LineWidth = lineWidth;
    }
}