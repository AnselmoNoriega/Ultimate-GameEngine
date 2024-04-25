#include "EditorLayer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "NotRed/Math/Math.h"
#include "NotRed/Utils/PlatformUtils.h"
#include "NotRed/Scene/SceneSerializer.h"
#include "NotRed/Scripting/ScriptEngine.h"

#include "imgui/imgui.h"
#include "ImGuizmo.h"

#include "NotRed/Math/Math.h"

namespace NR
{
    EditorLayer::EditorLayer()
        :Layer("EditorLayer")
    {
    }

    void EditorLayer::Attach()
    {
        NR_PROFILE_FUNCTION();

        mPlayIcon = Texture2D::Create("Resources/Icons/ToolBarUI/PlayButton.png");
        mStopIcon = Texture2D::Create("Resources/Icons/ToolBarUI/StopButton.png");
        mPauseIcon = Texture2D::Create("Resources/Icons/ToolBarUI/PauseButton.png");

        FramebufferStruct fbSpecs;
        fbSpecs.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
        fbSpecs.Width = 1600;
        fbSpecs.Height = 900;
        mFramebufferEditor = Framebuffer::Create(fbSpecs);
        mFramebufferGame = Framebuffer::Create(fbSpecs);

        mEditorScene = CreateRef<Scene>();
        mActiveScene = mEditorScene;
        mSceneHierarchyPanel.SetContext(mActiveScene);

        auto commandLineArgs = Application::Get().GetSpecification().CommandLineArgs;
        if (commandLineArgs.Count > 1)
        {
            auto projectFilePath = commandLineArgs[1];
            OpenProject(projectFilePath);
        }
        else if (!OpenProject())
        {
            Application::Get().Close();
        }

        mEditorCamera = EditorCamera(30.0f, 16 / 9, 0.1f, 1000.0f);
        Renderer2D::SetLineWidth(4.0f);
    }

    void EditorLayer::Detach()
    {
    }

    void EditorLayer::Update(float deltaTime)
    {
        NR_PROFILE_FUNCTION();

        if (FramebufferStruct spec = mFramebufferEditor->GetSpecification();
            mEditorViewportSize.x > 0.0f && mEditorViewportSize.y > 0.0f &&
            (spec.Width != mEditorViewportSize.x || spec.Height != mEditorViewportSize.y))
        {
            mFramebufferEditor->Resize((uint32_t)mEditorViewportSize.x, (uint32_t)mEditorViewportSize.y);
            mEditorCamera.SetViewportSize(mEditorViewportSize.x, mEditorViewportSize.y);
        }

        if (FramebufferStruct spec = mFramebufferGame->GetSpecification();
            mGameViewportSize.x > 0.0f && mGameViewportSize.y > 0.0f &&
            (spec.Width != mGameViewportSize.x || spec.Height != mGameViewportSize.y))
        {
            mFramebufferGame->Resize((uint32_t)mGameViewportSize.x, (uint32_t)mGameViewportSize.y);
            mActiveScene->ViewportResize((uint32_t)mGameViewportSize.x, (uint32_t)mGameViewportSize.y);
        }

        {
            NR_PROFILE_SCOPE("Render Start");
            Renderer2D::ResetStats();

            mFramebufferEditor->Bind();
            RenderCommand::SetClearColor({ 0.05f, 0.05f, 0.05f, 1 });
            RenderCommand::Clear();
            mFramebufferEditor->ClearAttachment(1, -1);
        }

        {
            NR_PROFILE_SCOPE("Rendering");

            if (mViewportFocused && mViewportHovered)
            {
                mEditorCamera.Update(deltaTime);
            }

            if (mSceneState == SceneState::Play)
            {
                mActiveScene->UpdateRunTime(deltaTime);
            }

            mActiveScene->UpdateEditor(deltaTime, mEditorCamera);

            auto [mx, my] = ImGui::GetMousePos();
            mx -= mViewportBounds[0].x;
            my -= mViewportBounds[0].y;
            glm::vec2 viewportSize = mViewportBounds[1] - mViewportBounds[0];
            my = viewportSize.y - my;

            if (mx >= 0.0f && my >= 0.0f && mx < viewportSize.x && my < viewportSize.y)
            {
                int pixelData = mFramebufferEditor->GetPixel(1, mx, my);
                mHoveredEntity = pixelData < -1 ? Entity() : Entity((entt::entity)pixelData, mActiveScene.get());
            }

            OverlayRender();
            mFramebufferEditor->Unbind();

            mFramebufferGame->Bind();
            RenderCommand::SetClearColor({ 0.05f, 0.05f, 0.05f, 1 });
            RenderCommand::Clear();
            mFramebufferGame->ClearAttachment(1, -1);

            mActiveScene->UpdatePlay(deltaTime);

            mFramebufferGame->Unbind();
        }
    }

    void EditorLayer::ImGuiRender()
    {
        NR_PROFILE_FUNCTION();

        static bool dockspaceOpen = true;
        static bool opt_fullscreen_persistant = true;
        bool opt_fullscreen = opt_fullscreen_persistant;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }
        // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;
        // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
        // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
        // all active windows docked into it will lose their parent and become undocked.
        // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
        // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();
        if (opt_fullscreen)
            ImGui::PopStyleVar(2);
        // DockSpace
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 200.0f;
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }
        style.WindowMinSize.x = minWinSizeX;

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open Project...", "Ctrl+Alt+O"))
                {
                    OpenProject();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("New Scene", "Ctrl+N"))
                {
                    NewScene();
                }
                if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
                {
                    OpenScene();
                }
                if (ImGui::MenuItem("Save", "Ctrl+S"))
                {
                    SaveScene();
                }
                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
                {
                    SaveSceneAs();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Exit"))
                {
                    Application::Get().Close();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Script"))
            {
                if (ImGui::MenuItem("Reload assembly", "Ctrl+R"))
                {
                    ScriptEngine::ReloadAssembly();
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        mSceneHierarchyPanel.ImGuiRender();
        mBrowserPanel->ImGuiRender();

        ImGui::Begin("Stats");

        std::string hoveredEntityName = "None";
        if (mHoveredEntity)
        {
            hoveredEntityName = mHoveredEntity.GetComponent<TagComponent>().Tag;
        }
        ImGui::Text("Hovered Entity: %s", hoveredEntityName.c_str());

        auto stats = Renderer2D::GetStats();
        ImGui::Text("Renderer Stats:");
        ImGui::Text("Draw Calls: %d", stats.DrawCalls);
        ImGui::Text("Quads: %d", stats.QuadCount);
        ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
        ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

        ImGui::End();

        ImGui::Begin("Settings");
        ImGui::Checkbox("Show physics colliders", &mShowPhysicsColliders);
        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0,0 });
        ImGui::Begin("Viewport Game");

        ImVec2 vpGameSize = ImGui::GetContentRegionAvail();
        mGameViewportSize = { vpGameSize.x, vpGameSize.y };

        uint32_t gameTextureID = mFramebufferGame->GetTextureRendererID();
        ImGui::Image((void*)gameTextureID, ImVec2{ mGameViewportSize.x, mGameViewportSize.y }, ImVec2{ 0,1 }, ImVec2{ 1,0 });

        ImGui::End();
        ImGui::PopStyleVar();

        ToolbarUI();

        ImGui::End();
        ImGui::Begin("Viewport Editor");

        auto minBound = ImGui::GetWindowContentRegionMin();
        auto maxBound = ImGui::GetWindowContentRegionMax();
        auto viewportOffset = ImGui::GetWindowPos();
        mViewportBounds[0] = { minBound.x + viewportOffset.x, minBound.y + viewportOffset.y };
        mViewportBounds[1] = { maxBound.x + viewportOffset.x, maxBound.y + viewportOffset.y };

        ImVec2 vpSize = ImGui::GetContentRegionAvail();
        mEditorViewportSize = { vpSize.x, vpSize.y };

        mViewportFocused = ImGui::IsWindowFocused();
        mViewportHovered = ImGui::IsWindowHovered();
        Application::Get().GetImGuiLayer()->SetEventsActive(!mViewportHovered);

        uint32_t textureID = mFramebufferEditor->GetTextureRendererID();
        ImGui::Image((void*)textureID, ImVec2{ mEditorViewportSize.x, mEditorViewportSize.y }, ImVec2{ 0,1 }, ImVec2{ 1,0 });

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                const wchar_t* path = (const wchar_t*)payload->Data;
                const wchar_t* fileExtension = wcsrchr(path, L'.');

                if (!wcscmp(fileExtension, L".nr"))
                {
                    OpenScene(path);
                }
                else if (!wcscmp(fileExtension, L".png"))
                {
                    std::filesystem::path texturePath(path);

                    if (mHoveredEntity && mHoveredEntity.HasComponent<SpriteRendererComponent>())
                    {
                        mHoveredEntity.GetComponent<SpriteRendererComponent>().Texture = Texture2D::Create(texturePath.string());
                    }
                    else if (!mHoveredEntity)
                    {
                        auto entity = mActiveScene->CreateEntity();
                        entity.AddComponent<SpriteRendererComponent>();
                        entity.GetComponent<SpriteRendererComponent>().Texture = Texture2D::Create(texturePath.string());
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        Entity selectedEntity = mSceneHierarchyPanel.GetSelectedEntity();
        if (selectedEntity && mGizmoType >= 0)
        {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();

            ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

            const glm::mat4& camProj = mEditorCamera.GetProjection();
            glm::mat4 cameraView = mEditorCamera.GetViewMatrix();
            auto& tc = selectedEntity.GetComponent<TransformComponent>();
            glm::mat4 transform = tc.GetTransform();

            bool snap = Input::IsKeyPressed(KeyCode::Left_Control);
            float snapValue = mGizmoType != ImGuizmo::OPERATION::ROTATE ? 1.0f : 45.0f;

            float snapValues[3] = { snapValue, snapValue, snapValue };

            ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(camProj),
                (ImGuizmo::OPERATION)mGizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform),
                nullptr, snap ? snapValues : nullptr);

            if (ImGuizmo::IsUsing())
            {
                glm::vec3 translation, rotation, scale;
                Math::DecomposeTransform(transform, translation, rotation, scale);

                glm::vec3 deltaRotation = rotation - tc.Rotation;
                tc.Translation = translation;
                tc.Rotation += deltaRotation;
                tc.Scale = scale;
            }
        }

        ImGui::End();
    }

    void EditorLayer::ToolbarUI()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        auto& colors = ImGui::GetStyle().Colors;

        const auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
        const auto& buttonActive = colors[ImGuiCol_ButtonActive];
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

        ImGui::Begin("##toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        float size = ImGui::GetWindowHeight() - 4.0f;
        float iconOffset = mSceneState == SceneState::Edit ? (size * 0.5f) : size;
        Ref<Texture2D> icon = mSceneState == SceneState::Edit ? mPlayIcon : mStopIcon;
        ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - iconOffset);

        if (ImGui::ImageButton((ImTextureID)icon->GetRendererID(), ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), 0))
        {
            if (mSceneState == SceneState::Edit)
            {
                PlayScene();
            }
            else if (mSceneState == SceneState::Play)
            {
                StopScene();
            }
        }

        if (mSceneState != SceneState::Edit)
        {
            Ref<Texture2D> pauseIcon = mActiveScene->IsPaused() ? mPlayIcon : mPauseIcon;

            ImGui::SameLine();
            if (ImGui::ImageButton((ImTextureID)pauseIcon->GetRendererID(), ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), 0))
            {
                mActiveScene->SetPaused(!mActiveScene->IsPaused());
            }
        }
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);
        ImGui::End();
    }

    void EditorLayer::OnEvent(Event& myEvent)
    {
        mEditorCamera.OnEvent(myEvent);

        EventDispatcher dispatch(myEvent);
        dispatch.Dispatch<KeyPressedEvent>(NR_BIND_EVENT_FN(EditorLayer::KeyPressed));
        dispatch.Dispatch<MouseButtonPressedEvent>(NR_BIND_EVENT_FN(EditorLayer::MouseButtonPressed));
    }

    void EditorLayer::OverlayRender()
    {
        Renderer2D::BeginScene(mEditorCamera);

        if (mShowPhysicsColliders)
        {
            {
                auto view = mActiveScene->GetAllEntitiesWith<TransformComponent, BoxCollider2DComponent>();
                for (auto entity : view)
                {
                    auto&& [tc, bc2d] = view.get<TransformComponent, BoxCollider2DComponent>(entity);

                    glm::vec3 translation = tc.Translation + glm::vec3(bc2d.Offset.x, bc2d.Offset.y, 0.001f);
                    glm::vec3 scale = tc.Scale * glm::vec3(bc2d.Size.x * 2.0f, bc2d.Size.y * 2.0f, 1.0f);

                    float rotation = tc.Rotation.z;
                    glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation)
                        * glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0.0f, 0.0f, 1.0f))
                        * glm::scale(glm::mat4(1.0f), scale);

                    Renderer2D::DrawRect(transform, glm::vec4(0.0f, 1.0f, 0.2f, 1.0f));
                }
            }
            {
                auto view = mActiveScene->GetAllEntitiesWith<TransformComponent, CircleCollider2DComponent>();
                for (auto entity : view)
                {
                    auto [tc, cc2d] = view.get<TransformComponent, CircleCollider2DComponent>(entity);

                    glm::vec3 translation = tc.Translation + glm::vec3(cc2d.Offset.x, cc2d.Offset.y, 0.001f);
                    float scale = tc.Scale.x * cc2d.Radius * 2.0f;

                    glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation)
                        * glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, 1.0f));

                    Renderer2D::DrawCircle(transform, glm::vec4(0.0f, 1.0f, 0.2f, 1.0f), 0.01f);
                }
            }
        }

        if (Entity selectedEntity = mSceneHierarchyPanel.GetSelectedEntity())
        {
            TransformComponent transform = selectedEntity.GetComponent<TransformComponent>();

            Renderer2D::DrawRect(transform.GetTransform(), glm::vec4(1, 0, 0, 1));
        }

        Renderer2D::EndScene();
    }

    bool EditorLayer::KeyPressed(KeyPressedEvent& e)
    {
        if (e.GetRepeatCount() > 0)
        {
            return false;
        }

        bool ctrl = Input::IsKeyPressed(KeyCode::Left_Control) || Input::IsKeyPressed(KeyCode::Right_Control);
        bool shift = Input::IsKeyPressed(KeyCode::Left_Shift) || Input::IsKeyPressed(KeyCode::Right_Shift);

        switch ((KeyCode)e.GetKeyCode())
        {
        case KeyCode::Delete:
        {
            if (Application::Get().GetImGuiLayer()->GetActiveWidgetID() == 0)
            {
                Entity selectedEntity = mSceneHierarchyPanel.GetSelectedEntity();
                if (selectedEntity)
                {
                    mSceneHierarchyPanel.SetSelectedEntity({});
                    mActiveScene->RemoveEntity(selectedEntity);
                }
            }
            break;
        }

        //Settings
        case KeyCode::N:
        {
            if (ctrl)
            {
                NewScene();
            }
            break;
        }
        case KeyCode::O:
        {
            if (ctrl && Input::IsKeyPressed(KeyCode::Left_Alt))
            {
                OpenProject();
            }
            else if (ctrl)
            {
                OpenScene();
            }
            break;
        }
        case KeyCode::S:
        {
            if (ctrl)
            {
                if (shift)
                {
                    SaveSceneAs();
                }
                else
                {
                    SaveScene();
                }
            }
            break;
        }
        case KeyCode::D:
        {
            if (ctrl)
            {
                DuplicateEntity();
            }

            break;
        }

        //Gizmos
        case KeyCode::Q:
        {
            mGizmoType = -1;
            break;
        }case KeyCode::W:
        {
            mGizmoType = ImGuizmo::OPERATION::TRANSLATE;
            break;
        }case KeyCode::E:
        {
            mGizmoType = ImGuizmo::OPERATION::SCALE;
            break;
        }case KeyCode::R:
        {
            if (ctrl)
            {
                ScriptEngine::ReloadAssembly();
            }
            else
            {
                mGizmoType = ImGuizmo::OPERATION::ROTATE;

            }

            break;
        }
        }

        return false;
    }

    bool EditorLayer::MouseButtonPressed(MouseButtonPressedEvent& e)
    {
        if (e.GetMouseButton() == (int)MouseCode::ButtonLeft)
        {
            if (mViewportHovered && !ImGuizmo::IsOver() && !Input::IsKeyPressed(KeyCode::Left_Alt))
            {
                mSceneHierarchyPanel.SetSelectedEntity(mHoveredEntity);
            }
        }

        return false;
    }

    void EditorLayer::DuplicateEntity()
    {
        Entity selectedEntity = mSceneHierarchyPanel.GetSelectedEntity();
        if (selectedEntity)
        {
            Entity newEntity = mEditorScene->DuplicateEntity(selectedEntity);
            mSceneHierarchyPanel.SetSelectedEntity(newEntity);
        }
    }

    void EditorLayer::NewProject()
    {
        Project::New();
    }

    bool EditorLayer::OpenProject()
    {
        std::string filepath = FileDialogs::OpenFile("NotRed Project (*.nrproj)\0*.nrproj\0");
        if (filepath.empty())
        {
            return false;
        }

        OpenProject(filepath);
        return true;
    }

    void EditorLayer::OpenProject(const std::filesystem::path& path)
    {
        if (Project::Load(path))
        {
            ScriptEngine::Init();

            auto startScenePath = Project::GetAssetFileSystemPath(Project::GetActive()->GetConfig().StartScene);
            OpenScene(startScenePath);
            mBrowserPanel = CreateScope<BrowserPanel>();
        }
    }

    void EditorLayer::SaveProject()
    {
    }

    void EditorLayer::SerializeScene(Ref<Scene> scene, const std::filesystem::path& path)
    {
        SceneSerializer serializer(scene);
        serializer.Serialize(path.string());
    }

    void EditorLayer::OpenScene()
    {
        std::string filepath = FileDialogs::OpenFile("NotRed Scene (*.nr)\0*.nr\0");

        if (!filepath.empty())
        {
            OpenScene(filepath);
        }
    }

    void EditorLayer::OpenScene(const std::filesystem::path& path)
    {
        if (mSceneState != SceneState::Edit)
        {
            StopScene();
        }

        mEditorScene = CreateRef<Scene>();
        SceneSerializer serializer(mEditorScene);
        if (serializer.Deserialize(path.string()))
        {
            mSceneHierarchyPanel.SetContext(mEditorScene);

            mActiveScene = mEditorScene;
            mScenePath = path;
        }
    }

    void EditorLayer::NewScene()
    {
        if (mSceneState != SceneState::Edit)
        {
            StopScene();
        }

        mEditorScene = CreateRef<Scene>();
        mEditorScene->ViewportResize((uint32_t)mEditorViewportSize.x, (uint32_t)mEditorViewportSize.y);
        mActiveScene = mEditorScene;
        mSceneHierarchyPanel.SetContext(mEditorScene);
        mScenePath = std::filesystem::path();
    }

    void EditorLayer::SaveSceneAs()
    {
        std::string filepath = FileDialogs::SaveFile("NotRed Scene (*.nr)\0*.nr\0");

        if (!filepath.empty())
        {
            SceneSerializer serializer(mActiveScene);
            serializer.Serialize(filepath);
        }
    }

    void EditorLayer::SaveScene()
    {
        if (!mScenePath.empty())
        {
            SerializeScene(mActiveScene, mScenePath);
        }
        else
        {
            SaveSceneAs();
        }
    }

    void EditorLayer::PlayScene()
    {
        mSceneState = SceneState::Play;

        mActiveScene = Scene::Copy(mEditorScene);
        mActiveScene->RuntimeStart();

        mSceneHierarchyPanel.SetContext(mActiveScene);
    }

    void EditorLayer::StopScene()
    {
        mSceneState = SceneState::Edit;

        mActiveScene->RuntimeStop();
        mActiveScene = mEditorScene;

        mSceneHierarchyPanel.SetContext(mEditorScene);
    }
}
