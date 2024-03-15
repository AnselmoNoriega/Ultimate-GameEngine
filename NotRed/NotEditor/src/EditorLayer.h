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
        Ref<Scene> mActiveScene;

        Ref<Texture2D> mTexture;
        Ref<Framebuffer> mFramebuffer;

        glm::vec2 mViewportSize = { 0.0f, 0.0f };
        bool mViewportFocused = false,
             mViewportHovered = false;

        glm::vec4 mSquareColor = { 0.8f, 0.3f, 0.2f, 1.0f };

        Entity mEntity;
        Entity mCameraEntity;
    };
}