#include "nrpch.h"
#include "Renderer.h"

#include "Platform/OpenGL/GLShader.h"
#include "Renderer2D.h"

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
        //Ref<UniformBuffer> CameraUniformBuffer;
    };

    void Renderer::SetMeshLayout(Ref<VertexArray> vertexArray, Ref<VertexBuffer> vertexBuffer, uint32_t verticesCount)
    {
        vertexArray = VertexArray::Create();

        vertexBuffer = VertexBuffer::Create(verticesCount * sizeof(QuadVertex));
        vertexBuffer->SetLayout(
            {
                {ShaderDataType::Float3, "aPosition"},
                {ShaderDataType::Float2, "aTexCoord"},
                {ShaderDataType::Float4, "aColor"   },
                {ShaderDataType::Float,  "aTexIndex"},
                {ShaderDataType::Int,    "aEntityID"}
            });
        vertexArray->AddVertexBuffer(vertexBuffer);
    }
}