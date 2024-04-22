#pragma once

#include "NotRed.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/BrowserPanel.h"

#include "NotRed/Renderer/EditorCamera.h"

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
        void OverlayRender();
        void ImGuiRender() override;

        void PlayScene();
        void StopScene();

        void ToolbarUI();

    private:
        bool KeyPressed(KeyPressedEvent& e);
        bool MouseButtonPressed(MouseButtonPressedEvent& e);

        void OpenScene();
        void OpenScene(const std::filesystem::path& path);
        void NewScene();
        void SaveSceneAs(); 
        void SaveScene();
        
        void SerializeScene(Ref<Scene> scene, const std::filesystem::path& path);

        void DuplicateEntity();

    private:
        Ref<Scene> mActiveScene;
        Ref<Scene> mEditorScene;
        std::filesystem::path mScenePath;
        SceneHierarchyPanel mSceneHierarchyPanel;
        BrowserPanel mBrowserPanel;

        enum class SceneState
        {
            Edit,
            Play
        };
        SceneState mSceneState = SceneState::Edit;

        EditorCamera mEditorCamera;

        Ref<Framebuffer> mFramebufferEditor;
        Ref<Framebuffer> mFramebufferGame;

        Ref<Texture2D> mPlayIcon, mStopIcon, mPauseIcon;

        glm::vec2 mGameViewportSize = { 0.0f, 0.0f };
        glm::vec2 mEditorViewportSize = { 0.0f, 0.0f };

        bool mViewportFocused = false,
            mViewportHovered = false;

        glm::vec2 mViewportBounds[2];

        int mGizmoType = -1;
        bool mShowPhysicsColliders = false;

        Entity mHoveredEntity;
    };
}