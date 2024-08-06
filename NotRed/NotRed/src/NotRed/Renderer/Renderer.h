#pragma once

#include "RenderCommand.h"

#include "Camera/OrthographicCamera.h"
#include "Shader.h"

namespace NR
{
    class Mesh;
    class Camera;
    class EditorCamera;
    class Texture2D;

    class Renderer
    {
    public:
        static void Init();
        static void Shutdown(); 
        
        static void OnWindowResize(uint32_t width, uint32_t height);

        static void BeginScene(OrthographicCamera& camera);

        static void BeginScene(const Camera& camera, glm::mat4 transform);
        static void BeginScene(const EditorCamera& camera);
        static void BeginScene(const OrthographicCamera& camera);
        static void EndScene();

        static void Flush();

        //static void DrawModel(const glm::mat4& transform, SpriteRendererComponent& src, int entityID);

        inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

        struct Statistics
        {
            uint32_t DrawCalls = 0;
            uint32_t QuadCount = 0;

            uint32_t GetTotalVertexCount() const { return QuadCount * 4; }
            uint32_t GetTotalIndexCount() const { return QuadCount * 6; }
        };
        static void ResetStats();
        static Statistics GetStats();

        static void SetMeshLayout(Ref<VertexArray> vertexArray, Ref<VertexBuffer> vertexBuffer, uint32_t verticesCount);
        static void PackageVertices(Ref<Mesh> model, glm::vec3 position, glm::vec2 texCoord);

    private:
        static float GetTextureIndex(const Ref<Texture2D>& texture);
 
        static void StartBatch();
        static void NextBatch();
    };
}