#pragma once

#include "OrthographicCamera.h"
#include "Camera.h"
#include "EditorCamera.h"

namespace NR
{
    class Texture2D;
    class SubTexture;
    class Camera;
    struct SpriteRendererComponent;

    class Renderer2D
    {
    public:
        static void Init();
        static void Shutdown();

        static void BeginScene(const Camera& camera, glm::mat4 transform);
        static void BeginScene(const EditorCamera& camera);
        static void BeginScene(const OrthographicCamera& camera);
        static void EndScene();

        static void Flush();

        static void DrawSprite(const glm::mat4& transform, SpriteRendererComponent& src, int entityID);
        static void DrawCircle(const glm::mat4& transform, const glm::vec4& color, float thickness, int entityID = -1);
        static void DrawLine(const glm::vec3& p0, glm::vec3& p1, const glm::vec4& color);
        static void DrawRect(const glm::mat4& transform, const glm::vec4& color);

        static void SetLineWidth(float width);

        struct Statistics
        {
            uint32_t DrawCalls = 0;
            uint32_t QuadCount = 0;

            uint32_t GetTotalVertexCount() const { return QuadCount * 4; }
            uint32_t GetTotalIndexCount() const { return QuadCount * 6; }
        };
        static void ResetStats();
        static Statistics GetStats();


    private:
        static void DrawQuad(const glm::mat4& transform, float textureIndex, const glm::vec4& color, int entityID = -1);

        static float GetTextureIndex(const Ref<Texture2D>& texture);

        static void StartBatch();
        static void NextBatch();
    };
}