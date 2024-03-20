#pragma once

#include "NotRed.h"
#include "Panels/SceneHierarchyPanel.h"

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
        bool KeyPressed(KeyPressedEvent& e);

        void OpenScene();
        void SaveSceneAs();
        void NewScene();

    private:
        Ref<Scene> mActiveScene;
        SceneHierarchyPanel mSceneHierarchyPanel;

        Ref<Framebuffer> mFramebuffer;

        glm::vec2 mViewportSize = { 0.0f, 0.0f };
        bool mViewportFocused = false,
            mViewportHovered = false;
    };
}