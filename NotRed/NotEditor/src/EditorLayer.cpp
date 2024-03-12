#include "EditorLayer.h"

#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"

namespace NR
{
    EditorLayer::EditorLayer()
        :Layer("EditorLayer"), mCameraController(1280.0f / 720.0f)
    {
    }

    void EditorLayer::Attach()
    {
        NR_PROFILE_FUNCTION();

        mTexture = Texture2D::Create("Assets/Textures/Image_Two.png");

        FramebufferStruct fbSpecs;
        fbSpecs.Width = 1280;
        fbSpecs.Height = 720;
        mFramebuffer = Framebuffer::Create(fbSpecs);

        mActiveScene = CreateRef<Scene>();
        auto entity = mActiveScene->CreateEntity();

        mActiveScene->Reg().emplace<TransformComponent>(entity);
        mActiveScene->Reg().emplace<SpriteRendererComponent>(entity);
    }

    void EditorLayer::Detach()
    {
    }

    void EditorLayer::Update(float deltaTime)
    {
        NR_PROFILE_FUNCTION();

        if (mViewportFocused)
        {
            mCameraController.Update(deltaTime);
        }

        {
            NR_PROFILE_SCOPE("Render Start");
            mFramebuffer->Bind();
            RenderCommand::SetClearColor({ 0.05f, 0, 0, 1 });
            RenderCommand::Clear();
        }

        {
            NR_PROFILE_SCOPE("Rendering");
            Renderer2D::BeginScene(mCameraController.GetCamera());

            mActiveScene->Update(deltaTime);

            Renderer2D::EndScene();
            mFramebuffer->Unbind();
        }
    }

    void EditorLayer::OnEvent(Event& myEvent)
    {
        if (mViewportFocused && mViewportHovered)
        {
            mCameraController.OnEvent(myEvent);
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
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                // Disabling fullscreen would allow the window to be moved to the front of other windows, 
                // which we can't undo at the moment without finer window depth/z control.
                //ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);
                if (ImGui::MenuItem("Exit")) Application::Get().Close();
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::Begin("Settings");

        ImGui::ColorEdit4("Square Color", glm::value_ptr(mSquareColor));
        ImGui::DragFloat2("Obj pos 1", &objPositions[0].x, 0.1f, -10.0f, 10.0f);
        ImGui::DragFloat2("Obj pos 2", &objPositions[1].x, 0.1f, -10.0f, 10.0f);

        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0,0 });
        ImGui::Begin("Viewport");

        mViewportFocused = ImGui::IsWindowFocused();
        mViewportHovered = ImGui::IsWindowHovered();
        if (mViewportFocused || mViewportHovered)
        {
            Application::Get().GetImGuiLayer()->SetEventsActive(!mViewportFocused || !mViewportHovered);

        }
        Application::Get().GetImGuiLayer()->SetEventsActive(!mViewportFocused || !mViewportHovered);

        ImVec2 vpSize = ImGui::GetContentRegionAvail();
        if (mViewportSize != *((glm::vec2*)&vpSize))
        {
            mViewportSize = { vpSize.x, vpSize.y };
            mFramebuffer->Resize((uint32_t)vpSize.x, (uint32_t)vpSize.y);

            mCameraController.Resize(vpSize.x, vpSize.y);
        }
        uint32_t textureID = mFramebuffer->GetTextureRendererID();
        ImGui::Image((void*)textureID, ImVec2{ mViewportSize.x, mViewportSize.y }, ImVec2{ 0,1 }, ImVec2{ 1,0 });

        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::End();
    }
}
