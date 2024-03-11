#pragma once

#include "NotRed.h"

namespace NR
{
    class EditorLayer : public Layer
    {
    public:
        EditorLayer();
        ~EditorLayer() override = default;

        void Attach();
        void Detach();

        void Update(float deltaTime) override;
        void OnEvent(Event& myEvent) override;
        void ImGuiRender() override;

    private:
        OrthographicCameraController mCameraController;

        Ref<Texture2D> mTexture;
        Ref<Framebuffer> mFramebuffer;

        glm::vec2 mViewportSize = { 0.0f, 0.0f };
        bool mViewportFocused = false,
             mViewportHovered = false;

        glm::vec4 mSquareColor = { 0.8f, 0.3f, 0.2f, 1.0f };

        glm::vec2 objPositions[2] = {
            { 1.0f, 0.0f },
            {-1.0f, 0.0f }
        };
    };
}