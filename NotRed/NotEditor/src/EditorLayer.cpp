#include "EditorLayer.h"

#include <filesystem>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/imgui_internal.h>

#include "imgui_internal.h"
#include "NotRed/ImGui/ImGui.h"

#include "NotRed/ImGui/ImGuizmo.h"
#include "NotRed/Renderer/Renderer2D.h"
#include "NotRed/Script/ScriptEngine.h"
#include "NotRed/Editor/AssetEditorPanel.h"

#include "NotRed/Physics/PhysicsManager.h"
#include "NotRed/Editor/PhysicsSettingsWindow.h"
#include "NotRed/Physics/Debug/PhysicsDebugger.h"

#include "NotRed/Util/FileSystem.h"
#include "NotRed/Math/Math.h"

#include "NotRed/Renderer/RendererAPI.h"
#include "NotRed/Platform/OpenGL/GLFramebuffer.h"

#include "NotRed/Audio/AudioEngine.h"
#include "NotRed/Audio/SceneAudio.h"

namespace NR
{
    EditorLayer::EditorLayer()
        : mSceneType(SceneType::Model), mEditorCamera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f))
    {
    }

    EditorLayer::~EditorLayer()
    {
    }

    void EditorLayer::Attach()
    {
        using namespace glm;

        mCheckerboardTex = Texture2D::Create("Assets/Editor/Checkerboard.tga");
        mPlayButtonTex = Texture2D::Create("Assets/Editor/PlayButton.png");
        mPauseButtonTex = Texture2D::Create("Assets/Editor/PauseButton.png");
        mStopButtonTex = Texture2D::Create("Assets/Editor/StopButton.png");

        mSceneHierarchyPanel = CreateScope<SceneHierarchyPanel>(mEditorScene);
        mSceneHierarchyPanel->SetSelectionChangedCallback(std::bind(&EditorLayer::SelectEntity, this, std::placeholders::_1));
        mSceneHierarchyPanel->SetEntityDeletedCallback(std::bind(&EditorLayer::EntityDeleted, this, std::placeholders::_1));

        mContentBrowserPanel = CreateScope<ContentBrowserPanel>();
        mObjectsPanel = CreateScope<ObjectsPanel>();

        NewScene();

        AssetEditorPanel::RegisterDefaultEditors();
    }

    void EditorLayer::Detach()
    {
        AssetEditorPanel::UnregisterAllEditors();
    }

    void EditorLayer::ScenePlay()
    {
        mSelectionContext.clear();
        mSceneState = SceneState::Play;

        if (mReloadScriptOnPlay)
        {
            ScriptEngine::ReloadAssembly("Assets/Scripts/ExampleApp.dll");
        }

        mRuntimeScene = Ref<Scene>::Create();
        mEditorScene->CopyTo(mRuntimeScene);

        mRuntimeScene->RuntimeStart();
        mSceneHierarchyPanel->SetContext(mRuntimeScene);
        mCurrentScene = mRuntimeScene;
    }

    void EditorLayer::SceneStop()
    {
        mRuntimeScene->RuntimeStop();
        mSceneState = SceneState::Edit;

        mRuntimeScene = nullptr;

        mSelectionContext.clear();
        ScriptEngine::SetSceneContext(mEditorScene);
        Audio::AudioEngine::SetSceneContext(mEditorScene);
        mSceneHierarchyPanel->SetContext(mEditorScene);
        mCurrentScene = mEditorScene;

        Input::SetCursorMode(CursorMode::Normal);
    }

    void EditorLayer::SceneStartSimulation()
    {
        mSelectionContext.clear();

        mSceneState = SceneState::Simulate;
        mSimulationScene = Ref<Scene>::Create();

        mEditorScene->CopyTo(mSimulationScene);
        mSimulationScene->SimulationStart();
        mSceneHierarchyPanel->SetContext(mSimulationScene);

        mCurrentScene = mSimulationScene;
    }

    void EditorLayer::SceneEndSimulation()
    {
        mSimulationScene->SimulationEnd();

        mSceneState = SceneState::Edit;
        mSimulationScene = nullptr;

        mSelectionContext.clear();

        ScriptEngine::SetSceneContext(mEditorScene);

        mSceneHierarchyPanel->SetContext(mEditorScene);

        mCurrentScene = mEditorScene;
    }

    void EditorLayer::UpdateWindowTitle(const std::string& sceneName)
    {
        std::string rendererAPI = RendererAPI::Current() == RendererAPIType::Vulkan ? "Vulkan" : "OpenGL";
        std::string title = sceneName + " - NotEditor - " + Application::GetPlatformName() + " (" + Application::GetConfigurationName() + ") Renderer: " + rendererAPI;
        Application::Get().GetWindow().SetTitle(title);
    }

    float EditorLayer::GetSnapValue()
    {
        switch (mGizmoType)
        {
        case  ImGuizmo::OPERATION::TRANSLATE: return 0.5f;
        case  ImGuizmo::OPERATION::ROTATE: return 45.0f;
        case  ImGuizmo::OPERATION::SCALE: return 0.5f;
        }
        return 0.0f;
    }

    void EditorLayer::DeleteEntity(Entity entity)
    {
        for (auto childId : entity.Children())
        {
            DeleteEntity(mEditorScene->FindEntityByID(childId));
        }

        mEditorScene->UnparentEntity(entity);
        mEditorScene->DestroyEntity(entity);
    }

    void EditorLayer::Update(float dt)
    {
        auto [x, y] = GetMouseViewportSpace();

        switch (mSceneState)
        {
        case SceneState::Edit:
        {
            mEditorCamera.Update(dt);

            mEditorScene->RenderEditor(dt, mEditorCamera);

            if (mDrawOnTopBoundingBoxes)
            {
                Renderer::BeginRenderPass(SceneRenderer::GetFinalRenderPass(), false);
                auto viewProj = mEditorCamera.GetViewProjection();
                Renderer2D::BeginScene(viewProj, false);
                Renderer2D::EndScene();
                Renderer::EndRenderPass();
            }

            if (mSelectionContext.size() && false)
            {
                auto& selection = mSelectionContext[0];
                if (selection.Mesh && selection.EntityObj.HasComponent<MeshComponent>())
                {
                    Renderer::BeginRenderPass(SceneRenderer::GetFinalRenderPass(), false);
                    auto viewProj = mEditorCamera.GetViewProjection();
                    Renderer2D::BeginScene(viewProj, false);
                    glm::vec4 color = (mSelectionMode == SelectionMode::Entity) ? glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f } : glm::vec4{ 0.2f, 0.9f, 0.2f, 1.0f };
                    Renderer::DrawAABB(selection.Mesh->BoundingBox, selection.EntityObj.Transform().GetTransform() * selection.Mesh->Transform, color);
                    Renderer2D::EndScene();
                    Renderer::EndRenderPass();
                }
            }

            if (mSelectionContext.size())
            {
                auto& selection = mSelectionContext[0];

                if (selection.EntityObj.HasComponent<BoxCollider2DComponent>() && false)
                {
                    const auto& size = selection.EntityObj.GetComponent<BoxCollider2DComponent>().Size;
                    const TransformComponent& transform = selection.EntityObj.GetComponent<TransformComponent>();

                    Renderer::BeginRenderPass(SceneRenderer::GetFinalRenderPass(), false);
                    auto viewProj = mEditorCamera.GetViewProjection();
                    Renderer2D::BeginScene(viewProj, false);
                    Renderer2D::DrawRotatedRect({ transform.Translation.x, transform.Translation.y }, size * 2.0f, transform.Rotation.z, { 0.0f, 1.0f, 1.0f, 1.0f });
                    Renderer2D::EndScene();
                    Renderer::EndRenderPass();
                }

                if (selection.EntityObj.HasComponent<CircleCollider2DComponent>() && false)
                {
                    const auto& size = selection.EntityObj.GetComponent<CircleCollider2DComponent>().Radius;
                    const TransformComponent& transform = selection.EntityObj.GetComponent<TransformComponent>();

                    Renderer::BeginRenderPass(SceneRenderer::GetFinalRenderPass(), false);
                    auto viewProj = mEditorCamera.GetViewProjection();
                    Renderer2D::BeginScene(viewProj, false);
                    Renderer2D::DrawCircle({ transform.Translation.x, transform.Translation.y }, size, { 0.0f, 1.0f, 1.0f, 1.0f });
                    Renderer2D::EndScene();
                    Renderer::EndRenderPass();
                }
            }
            break;
        }
        case SceneState::Play:
        {
            if (mViewportPanelFocused)
            {
                mEditorCamera.Update(dt);
            }

            mRuntimeScene->Update(dt);
            mRuntimeScene->RenderRuntime(dt);
            break;
        }
        case SceneState::Pause:
        {
            if (mViewportPanelFocused)
            {
                mEditorCamera.Update(dt);
            }

            mRuntimeScene->RenderRuntime(dt);
            break;
        }
        case SceneState::Simulate:
        {
            mEditorCamera.Update(dt);
            mSimulationScene->Update(dt);
            mSimulationScene->RenderSimulation(dt, mEditorCamera);
            break;
        }
        }
    }

    void EditorLayer::NewScene()
    {
        mSelectionContext = {};

        mEditorScene = Ref<Scene>::Create("Scene", true);
        mSceneHierarchyPanel->SetContext(mEditorScene);
        ScriptEngine::SetSceneContext(mEditorScene);
        Audio::AudioEngine::SetSceneContext(mEditorScene);
        UpdateWindowTitle("Scene");
        mSceneFilePath = std::string();

        mEditorCamera = EditorCamera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f));
        mCurrentScene = mEditorScene;
    }

    void EditorLayer::OpenScene()
    {
        auto& app = Application::Get();
        std::string filepath = app.OpenFile("NotRed Scene (*.nsc)\0*.nsc\0");
        if (!filepath.empty())
        {
            OpenScene(filepath);
        }
    }

    void EditorLayer::OpenScene(const std::string& filepath)
    {
        Ref<Scene> newScene = Ref<Scene>::Create();
        SceneSerializer serializer(newScene);
        serializer.Deserialize(filepath);

        mEditorScene = newScene;
        mSceneFilePath = filepath;
        std::filesystem::path path = filepath;

        UpdateWindowTitle(path.filename().string());
        mSceneHierarchyPanel->SetContext(mEditorScene);
        ScriptEngine::SetSceneContext(mEditorScene);
        Audio::AudioEngine::SetSceneContext(mEditorScene);

        mEditorScene->SetSelectedEntity({});
        mSelectionContext.clear();

        mCurrentScene = mEditorScene;
    }

    void EditorLayer::SaveScene()
    {
        if (!mSceneFilePath.empty())
        {
            SceneSerializer serializer(mEditorScene);
            serializer.Serialize(mSceneFilePath);
        }
        else
        {
            SaveSceneAs();
        }
    }

    void EditorLayer::SaveSceneAs()
    {
        auto& app = Application::Get();
        std::string filepath = app.SaveFile("NotRed Scene (*.nsc)\0*.nsc\0\0");
        if (!filepath.empty())
        {
            if (filepath.find(".nsc") == std::string::npos)
            {
                filepath += ".nsc";
            }

            SceneSerializer serializer(mEditorScene);
            serializer.Serialize(filepath);

            std::filesystem::path path = filepath;
            UpdateWindowTitle(path.filename().string());
            mSceneFilePath = filepath;
        }
    }

    void EditorLayer::ImGuiRender()
    {
        static bool p_open = true;

        static bool optFullscreenPersistant = true;
        static ImGuiDockNodeFlags optFlags = ImGuiDockNodeFlags_None;
        bool optFullscreen = optFullscreenPersistant;

        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        auto boldFont = io.Fonts->Fonts[0];
        auto largeFont = io.Fonts->Fonts[1];

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (optFullscreen)
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

        // When using ImGuiDockNodeFlags_PassthruDockspace, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
        ImGuiDockNodeFlags ImGuiDockNodeFlags_PassthruDockspace = ImGuiDockNodeFlags_NoSplit | ImGuiDockNodeFlags_NoResize | ImGuiDockNodeFlags_NoDockingInCentralNode;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &p_open, window_flags);
        ImGui::PopStyleVar();

        if (optFullscreen)
        {
            ImGui::PopStyleVar(2);
        }

        // Dockspace
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 370.0f;
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), optFlags);
        }

        style.WindowMinSize.x = minWinSizeX;

        // Editor Panel ------------------------------------------------------------------------------
        ImGui::Begin("Settings");
        {
            auto& rendererConfig = Renderer::GetConfig();

            UI::BeginPropertyGrid();
            ImGui::AlignTextToFramePadding();
            UI::PropertySlider("Skybox LOD", mEditorScene->GetSkyboxLod(), 0.0f, Utils::CalculateMipCount(rendererConfig.EnvironmentMapResolution, rendererConfig.EnvironmentMapResolution));

            UI::PropertySlider("Exposure", mEditorCamera.GetExposure(), 0.0f, 5.0f);

            UI::PropertySlider("Env Map Rotation", mEnvMapRotation, -360.0f, 360.0f);

            if (mSceneState == SceneState::Edit)
            {
                float physics2DGravity = mEditorScene->GetPhysics2DGravity();
                if (UI::Property("Gravity", physics2DGravity, -10000.0f, 10000.0f))
                {
                    mEditorScene->SetPhysics2DGravity(physics2DGravity);
                }
            }
            else if (mSceneState == SceneState::Play)
            {
                float physics2DGravity = mRuntimeScene->GetPhysics2DGravity();
                if (UI::Property("Gravity", physics2DGravity, -10000.0f, 10000.0f))
                {
                    mRuntimeScene->SetPhysics2DGravity(physics2DGravity);
                }
            }

            if (UI::Property("Show Bounding Boxes", mUIShowBoundingBoxes))
            {
                ShowBoundingBoxes(mUIShowBoundingBoxes, mUIShowBoundingBoxesOnTop);
            }
            if (mUIShowBoundingBoxes && UI::Property("On Top", mUIShowBoundingBoxesOnTop))
            {
                ShowBoundingBoxes(mUIShowBoundingBoxes, mUIShowBoundingBoxesOnTop);
            }

            const char* label = mSelectionMode == SelectionMode::Entity ? "Entity" : "Mesh";
            if (ImGui::Button(label))
            {
                mSelectionMode = mSelectionMode == SelectionMode::Entity ? SelectionMode::SubMesh : SelectionMode::Entity;
            }

            UI::EndPropertyGrid();

            ImGui::Separator();
            ImGui::PushFont(boldFont);
            ImGui::Text("Renderer Settings");
            ImGui::PopFont();
            UI::BeginPropertyGrid();
            UI::Property("Enable HDR environment maps", rendererConfig.ComputeEnvironmentMaps);
            {
                const char* environmentMapSizes[] = { "128", "256", "512", "1024", "2048", "4096" };
                int currentSize = (int)glm::log2((float)rendererConfig.EnvironmentMapResolution) - 7;
                if (UI::PropertyDropdown("Environment Map Size", environmentMapSizes, 6, &currentSize))
                {
                    rendererConfig.EnvironmentMapResolution = glm::pow(2, currentSize + 7);
                }
            }
            {
                const char* irradianceComputeSamples[] = { "128", "256", "512", "1024", "2048", "4096" };
                int currentSamples = (int)glm::log2((float)rendererConfig.IrradianceMapComputeSamples) - 7;
                if (UI::PropertyDropdown("Irradiance Map Compute Samples", irradianceComputeSamples, 6, &currentSamples))
                {
                    rendererConfig.IrradianceMapComputeSamples = glm::pow(2, currentSamples + 7);
                }
            }
            UI::EndPropertyGrid();
        }
        ImGui::End();

        mContentBrowserPanel->ImGuiRender();
        mObjectsPanel->ImGuiRender();
        AssetEditorPanel::ImGuiRender();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.305f, 0.31f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.1505f, 0.151f, 0.5f));

        ImGui::Begin("##tool_bar", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            float size = ImGui::GetWindowHeight() - 4.0f;
            ImGui::SameLine((ImGui::GetWindowContentRegionMax().x / 2.0f) - (1.5f * (ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.x)) - (size / 2.0f));
            Ref<Texture2D> buttonTex = mSceneState == SceneState::Play ? mStopButtonTex : mPlayButtonTex;
            if (UI::ImageButton(buttonTex, ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), 0))
            {
                if (mSceneState == SceneState::Edit)
                {
                    ScenePlay();
                }
                else
                {
                    SceneStop();
                }
            }

            ImGui::SameLine();

            if (UI::ImageButton(mPauseButtonTex, ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), 0))
            {
                if (mSceneState == SceneState::Play)
                {
                    mSceneState = SceneState::Pause;
                }
                else if (mSceneState == SceneState::Pause)
                {
                    mSceneState = SceneState::Play;
                }
            }

            ImGui::SameLine();
            ImGui::PushFont(boldFont);
            if (ImGui::Button("Simulate", ImVec2(0, size)))
            {
                if (mSceneState == SceneState::Edit)
                {
                    SceneStartSimulation();
                }
                else
                {
                    SceneEndSimulation();
                }
            }

            ImGui::PopFont();
        }
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);

        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport");

        mViewportPanelMouseOver = ImGui::IsWindowHovered();
        mViewportPanelFocused = ImGui::IsWindowFocused();

        auto viewportOffset = ImGui::GetCursorPos();
        auto viewportSize = ImGui::GetContentRegionAvail();

        SceneRenderer::SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
        mEditorScene->SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
        if (mRuntimeScene)
        {
            mRuntimeScene->SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
        }
        mEditorCamera.SetProjectionMatrix(glm::perspectiveFov(glm::radians(45.0f), viewportSize.x, viewportSize.y, 0.1f, 1000.0f));
        mEditorCamera.SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
        UI::Image(SceneRenderer::GetFinalPassImage(), viewportSize, { 0, 1 }, { 1, 0 });

        static int counter = 0;
        auto windowSize = ImGui::GetWindowSize();
        ImVec2 minBound = ImGui::GetWindowPos();
        minBound.x += viewportOffset.x;
        minBound.y += viewportOffset.y;

        ImVec2 maxBound = { minBound.x + windowSize.x, minBound.y + windowSize.y };
        mViewportBounds[0] = { minBound.x, minBound.y };
        mViewportBounds[1] = { maxBound.x, maxBound.y };
        mAllowViewportCameraEvents = ImGui::IsMouseHoveringRect(minBound, maxBound);

        if (mGizmoType != -1 && mSelectionContext.size())
        {
            auto& selection = mSelectionContext[0];

            float rw = (float)ImGui::GetWindowWidth();
            float rh = (float)ImGui::GetWindowHeight();
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, rw, rh);

            bool snap = Input::IsKeyPressed(NR_KEY_LEFT_CONTROL);

            TransformComponent& entityTransform = selection.EntityObj.Transform();
            glm::mat4 transform = mCurrentScene->GetTransformRelativeToParent(selection.EntityObj);
            float snapValue = GetSnapValue();
            float snapValues[3] = { snapValue, snapValue, snapValue };
            if (mSelectionMode == SelectionMode::Entity)
            {
                glm::mat4 transform = entityTransform.GetTransform();
                ImGuizmo::Manipulate(glm::value_ptr(mEditorCamera.GetViewMatrix()),
                    glm::value_ptr(mEditorCamera.GetProjectionMatrix()),
                    (ImGuizmo::OPERATION)mGizmoType,
                    ImGuizmo::LOCAL,
                    glm::value_ptr(transform),
                    nullptr,
                    snap ? snapValues : nullptr);

                if (ImGuizmo::IsUsing())
                {
                    Entity parent = mCurrentScene->FindEntityByID(selection.EntityObj.GetParentID());
                    if (parent)
                    {
                        glm::mat4 parentTransform = mCurrentScene->GetTransformRelativeToParent(parent);
                        transform = glm::inverse(parentTransform) * transform;
                        glm::vec3 translation, rotation, scale;
                        Math::DecomposeTransform(transform, translation, rotation, scale);

                        glm::vec3 deltaRotation = rotation - entityTransform.Rotation;
                        entityTransform.Translation = translation;
                        entityTransform.Rotation += deltaRotation;
                        entityTransform.Scale = scale;
                    }
                    else
                    {
                        glm::vec3 translation, rotation, scale;
                        Math::DecomposeTransform(transform, translation, rotation, scale);

                        glm::vec3 deltaRotation = rotation - entityTransform.Rotation;
                        entityTransform.Translation = translation;
                        entityTransform.Rotation += deltaRotation;
                        entityTransform.Scale = scale;
                    }
                }
            }
            else
            {
                glm::mat4 transformBase = transform * selection.Mesh->Transform;
                ImGuizmo::Manipulate(glm::value_ptr(mEditorCamera.GetViewMatrix()),
                    glm::value_ptr(mEditorCamera.GetProjectionMatrix()),
                    (ImGuizmo::OPERATION)mGizmoType,
                    ImGuizmo::LOCAL,
                    glm::value_ptr(transformBase),
                    nullptr,
                    snap ? snapValues : nullptr);

                selection.Mesh->Transform = glm::inverse(transform) * transformBase;
            }
        }

        /* Payload Implementation For Getting Assets In The Viewport From Asset Manager */
        if (ImGui::BeginDragDropTarget())
        {
            auto data = ImGui::AcceptDragDropPayload("asset_payload");
            if (data)
            {
                int count = data->DataSize / sizeof(AssetHandle);

                for (int i = 0; i < count; ++i)
                {
                    AssetHandle assetHandle = *(((AssetHandle*)data->Data) + i);
                    Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);

                    if (count == 1 && asset->Type == AssetType::Scene)
                    {
                        OpenScene(asset->FilePath);
                    }

                    if (asset->Type == AssetType::Mesh)
                    {
                        Entity entity = mEditorScene->CreateEntity(asset->FileName);
                        entity.AddComponent<MeshComponent>(Ref<Mesh>(asset));
                        SelectEntity(entity);
                    }
                }
            }

            ImGui::EndDragDropTarget();
        }

        ImGui::End();
        ImGui::PopStyleVar();

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Scene", "Ctrl+N"))
                {
                    NewScene();
                }
                if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
                {
                    OpenScene();
                }
                ImGui::Separator();

                if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
                {
                    SaveScene();
                }
                if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
                {
                    SaveSceneAs();
                }

                ImGui::Separator();
                std::string otherRenderer = RendererAPI::Current() == RendererAPIType::Vulkan ? "OpenGL" : "Vulkan";
                std::string label = std::string("Restart with ") + otherRenderer;
                if (ImGui::MenuItem(label.c_str()))
                {
                    RendererAPI::SetAPI(RendererAPI::Current() == RendererAPIType::Vulkan ? RendererAPIType::OpenGL : RendererAPIType::Vulkan);
                    Application::Get().Close();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Exit"))
                {
                    p_open = false;
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Script"))
            {
                if (ImGui::MenuItem("Reload C# Assembly"))
                {
                    ScriptEngine::ReloadAssembly("assets/scripts/ExampleApp.dll");
                }

                ImGui::MenuItem("Reload assembly on play", nullptr, &mReloadScriptOnPlay);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                ImGui::MenuItem("Physics Settings", nullptr, &mShowPhysicsSettings);

                ImGui::EndMenu();
            }

#ifdef NR_DEBUG
            if (ImGui::BeginMenu("Debugging"))
            {
                if (PhysicsDebugger::IsDebugging())
                {
                    if (ImGui::MenuItem("Stop Physics Debugging"))
                    {
                        PhysicsDebugger::StopDebugging();
                    }
                }
                else
                {
                    if (ImGui::MenuItem("Start Physics Debugging"))
                    {
                        PhysicsDebugger::StartDebugging("PhysXDebugInfo");
                    }
                }
                ImGui::EndMenu();
            }
#endif

            ImGui::EndMenuBar();
        }

        mSceneHierarchyPanel->ImGuiRender();

        ImGui::Begin("Materials");

        if (mSelectionContext.size())
        {
            Entity selectedEntity = mSelectionContext.front().EntityObj;
            if (selectedEntity.HasComponent<MeshComponent>())
            {
                Ref<Mesh> mesh = selectedEntity.GetComponent<MeshComponent>().MeshObj;
                if (mesh && mesh->Type == AssetType::Mesh)
                {
                    auto& materials = mesh->GetMaterials();
                    static uint32_t selectedMaterialIndex = 0;
                    for (uint32_t i = 0; i < materials.size(); ++i)
                    {
                        auto& materialInstance = materials[i];

                        ImGuiTreeNodeFlags node_flags = (selectedMaterialIndex == i ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_Leaf;
                        bool opened = ImGui::TreeNodeEx((void*)(&materialInstance), node_flags, materialInstance->GetName().c_str());
                        if (ImGui::IsItemClicked())
                        {
                            selectedMaterialIndex = i;
                        }
                        if (opened)
                        {
                            ImGui::TreePop();
                        }

                    }

                    ImGui::Separator();

                    if (selectedMaterialIndex < materials.size())
                    {
                        auto& materialInstance = materials[selectedMaterialIndex];
                        ImGui::Text("Shader: %s", materialInstance->GetShader()->GetName().c_str());
                        {
                            // Albedo
                            if (ImGui::CollapsingHeader("Albedo", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                            {
                                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

                                auto& albedoColor = materialInstance->GetVector3("uMaterialUniforms.AlbedoColor");
                                bool useAlbedoMap = true;
                                Ref<Texture2D> albedoMap = materialInstance->TryGetTexture2D("uAlbedoTexture");
                                bool hasAlbedoMap = !albedoMap.EqualsObject(Renderer::GetWhiteTexture()) && albedoMap->GetImage();
                                Ref<Texture2D> albedoUITexture = hasAlbedoMap ? albedoMap : mCheckerboardTex;
                                UI::Image(albedoUITexture, ImVec2(64, 64));

                                if (ImGui::BeginDragDropTarget())
                                {
                                    auto data = ImGui::AcceptDragDropPayload("asset_payload");
                                    if (data)
                                    {
                                        int count = data->DataSize / sizeof(AssetHandle);

                                        for (int i = 0; i < count; ++i)
                                        {
                                            if (count > 1)
                                            {
                                                break;
                                            }

                                            AssetHandle assetHandle = *(((AssetHandle*)data->Data) + i);
                                            Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
                                            if (asset->Type != AssetType::Texture)
                                            {
                                                break;
                                            }

                                            albedoMap = asset.As<Texture2D>();
                                            materialInstance->Set("uAlbedoTexture", albedoMap);
                                        }
                                    }

                                    ImGui::EndDragDropTarget();
                                }
                                ImGui::PopStyleVar();
                                if (ImGui::IsItemHovered())
                                {
                                    if (hasAlbedoMap)
                                    {
                                        ImGui::BeginTooltip();
                                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                        ImGui::TextUnformatted(albedoMap->GetPath().c_str());
                                        ImGui::PopTextWrapPos();
                                        UI::Image(albedoUITexture, ImVec2(384, 384));
                                        ImGui::EndTooltip();
                                    }
                                    if (ImGui::IsItemClicked())
                                    {
                                        std::string filename = Application::Get().OpenFile("");
                                        if (!filename.empty())
                                        {
                                            TextureProperties props;
                                            props.StandardRGB = true;
                                            albedoMap = Texture2D::Create(filename, props);
                                            materialInstance->Set("uAlbedoTexture", albedoMap);
                                        }
                                    }
                                }
                                ImGui::SameLine();
                                ImGui::BeginGroup();
                                ImGui::Checkbox("Use##AlbedoMap", &useAlbedoMap);

                                ImGui::EndGroup();
                                ImGui::SameLine();
                                ImGui::ColorEdit3("Color##Albedo", glm::value_ptr(albedoColor), ImGuiColorEditFlags_NoInputs);
                            }
                        }
                        {
                            // Normals
                            if (ImGui::CollapsingHeader("Normals", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                            {
                                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                                bool useNormalMap = materialInstance->GetFloat("uMaterialUniforms.UseNormalMap");
                                Ref<Texture2D> normalMap = materialInstance->TryGetTexture2D("uNormalTexture");
                                UI::Image((normalMap && normalMap->GetImage()) ? normalMap : mCheckerboardTex, ImVec2(64, 64));

                                if (ImGui::BeginDragDropTarget())
                                {
                                    auto data = ImGui::AcceptDragDropPayload("asset_payload");
                                    if (data)
                                    {
                                        int count = data->DataSize / sizeof(AssetHandle);

                                        for (int i = 0; i < count; ++i)
                                        {
                                            if (count > 1)
                                            {
                                                break;
                                            }

                                            AssetHandle assetHandle = *(((AssetHandle*)data->Data) + i);
                                            Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
                                            if (asset->Type != AssetType::Texture)
                                            {
                                                break;
                                            }

                                            normalMap = asset.As<Texture2D>();
                                            materialInstance->Set("uNormalTexture", normalMap);
                                            materialInstance->Set("uMaterialUniforms.UseNormalMap", true);
                                        }
                                    }

                                    ImGui::EndDragDropTarget();
                                }
                                ImGui::PopStyleVar();
                                if (ImGui::IsItemHovered())
                                {
                                    if (normalMap)
                                    {
                                        ImGui::BeginTooltip();
                                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                        ImGui::TextUnformatted(normalMap->GetPath().c_str());
                                        ImGui::PopTextWrapPos();
                                        UI::Image(normalMap, ImVec2(384, 384));
                                        ImGui::EndTooltip();
                                    }
                                    if (ImGui::IsItemClicked())
                                    {
                                        std::string filename = Application::Get().OpenFile("");
                                        if (filename != "")
                                        {
                                            normalMap = Texture2D::Create(filename);
                                            materialInstance->Set("uNormalTexture", normalMap);
                                        }
                                    }
                                }
                                ImGui::SameLine();
                                if (ImGui::Checkbox("Use##NormalMap", &useNormalMap))
                                {
                                    materialInstance->Set("uMaterialUniforms.UseNormalMap", useNormalMap);
                                }
                            }
                        }
                        {
                            // Metalness
                            if (ImGui::CollapsingHeader("Metalness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                            {
                                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                                float& metalnessValue = materialInstance->GetFloat("uMaterialUniforms.Metalness");
                                bool useMetalnessMap = true;
                                Ref<Texture2D> metalnessMap = materialInstance->TryGetTexture2D("uMetalnessTexture");
                                UI::Image((metalnessMap&& metalnessMap->GetImage()) ? metalnessMap : mCheckerboardTex, ImVec2(64, 64));

                                if (ImGui::BeginDragDropTarget())
                                {
                                    auto data = ImGui::AcceptDragDropPayload("asset_payload");
                                    if (data)
                                    {
                                        int count = data->DataSize / sizeof(AssetHandle);

                                        for (int i = 0; i < count; ++i)
                                        {
                                            if (count > 1)
                                            {
                                                break;
                                            }

                                            AssetHandle assetHandle = *(((AssetHandle*)data->Data) + i);
                                            Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
                                            if (asset->Type != AssetType::Texture)
                                            {
                                                break;
                                            }

                                            metalnessMap = asset.As<Texture2D>();
                                            materialInstance->Set("uMetalnessTexture", metalnessMap);
                                        }
                                    }

                                    ImGui::EndDragDropTarget();
                                }
                                ImGui::PopStyleVar();
                                if (ImGui::IsItemHovered())
                                {
                                    if (metalnessMap)
                                    {
                                        ImGui::BeginTooltip();
                                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                        ImGui::TextUnformatted(metalnessMap->GetPath().c_str());
                                        ImGui::PopTextWrapPos();
                                        UI::Image(metalnessMap, ImVec2(384, 384));
                                        ImGui::EndTooltip();
                                    }
                                    if (ImGui::IsItemClicked())
                                    {
                                        std::string filename = Application::Get().OpenFile("");
                                        if (filename != "")
                                        {
                                            metalnessMap = Texture2D::Create(filename);
                                            materialInstance->Set("uMetalnessTexture", metalnessMap);
                                        }
                                    }
                                }
                                ImGui::SameLine();
                                ImGui::Checkbox("Use##MetalnessMap", &useMetalnessMap);

                                ImGui::SameLine();
                                ImGui::SliderFloat("Value##MetalnessInput", &metalnessValue, 0.0f, 1.0f);
                            }
                        }
                        {
                            // Roughness
                            if (ImGui::CollapsingHeader("Roughness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                            {
                                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                                float& roughnessValue = materialInstance->GetFloat("uMaterialUniforms.Roughness");
                                bool useRoughnessMap = true;
                                Ref<Texture2D> roughnessMap = materialInstance->TryGetTexture2D("uRoughnessTexture");
                                UI::Image((roughnessMap&& roughnessMap->GetImage()) ? roughnessMap : mCheckerboardTex, ImVec2(64, 64));

                                if (ImGui::BeginDragDropTarget())
                                {
                                    auto data = ImGui::AcceptDragDropPayload("asset_payload");
                                    if (data)
                                    {
                                        int count = data->DataSize / sizeof(AssetHandle);

                                        for (int i = 0; i < count; ++i)
                                        {
                                            if (count > 1)
                                            {
                                                break;
                                            }

                                            AssetHandle assetHandle = *(((AssetHandle*)data->Data) + i);
                                            Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
                                            if (asset->Type != AssetType::Texture)
                                            {
                                                break;
                                            }

                                            roughnessMap = asset.As<Texture2D>();
                                            materialInstance->Set("uRoughnessTexture", roughnessMap);
                                        }
                                    }

                                    ImGui::EndDragDropTarget();
                                }
                                ImGui::PopStyleVar();
                                if (ImGui::IsItemHovered())
                                {
                                    if (roughnessMap)
                                    {
                                        ImGui::BeginTooltip();
                                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                        ImGui::TextUnformatted(roughnessMap->GetPath().c_str());
                                        ImGui::PopTextWrapPos();
                                        UI::Image(roughnessMap, ImVec2(384, 384));
                                        ImGui::EndTooltip();
                                    }
                                    if (ImGui::IsItemClicked())
                                    {
                                        std::string filename = Application::Get().OpenFile("");
                                        if (filename != "")
                                        {
                                            roughnessMap = Texture2D::Create(filename);
                                            materialInstance->Set("uRoughnessTexture", roughnessMap);
                                        }
                                    }
                                }
                                ImGui::SameLine();
                                ImGui::Checkbox("Use##RoughnessMap", &useRoughnessMap);
                                ImGui::SameLine();
                                ImGui::SliderFloat("Value##RoughnessInput", &roughnessValue, 0.0f, 1.0f);
                            }
                        }
                    }
                }
            }
        }

        ImGui::End();

        ScriptEngine::ImGuiRender();
        SceneRenderer::ImGuiRender();
        Audio::SceneAudio::ImGuiRender();
        PhysicsSettingsWindow::ImGuiRender(mShowPhysicsSettings);

        ImGui::End();

        if (mShowWelcomePopup)
        {
            ImGui::OpenPopup("Welcome");
            mShowWelcomePopup = false;
        }
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2{ 400,0 });
        if (ImGui::BeginPopupModal("Welcome", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Welcome to Not-Red!");
            ImGui::Separator();
            ImGui::TextWrapped("Environment maps are currently disabled.");
            UI::BeginPropertyGrid();
            UI::Property("Enable environment maps?", Renderer::GetConfig().ComputeEnvironmentMaps);
            UI::EndPropertyGrid();
            if (ImGui::Button("OK"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void EditorLayer::ShowBoundingBoxes(bool show, bool onTop)
    {
        SceneRenderer::GetOptions().ShowBoundingBoxes = show && !onTop;
        mDrawOnTopBoundingBoxes = show && onTop;
    }


    void EditorLayer::SelectEntity(Entity entity)
    {
        if (!entity)
        {
            return;
        }

        SelectedSubmesh selection;
        if (entity.HasComponent<MeshComponent>())
        {
            auto& meshComp = entity.GetComponent<MeshComponent>();

            if (meshComp.MeshObj && meshComp.MeshObj->Type == AssetType::Mesh)
            {
                selection.Mesh = &meshComp.MeshObj->GetSubmeshes()[0];
            }
        }
        selection.EntityObj = entity;
        mSelectionContext.clear();
        mSelectionContext.push_back(selection);

        if (mSceneState == SceneState::Edit)
        {
            mEditorScene->SetSelectedEntity(entity);
            mCurrentScene = mEditorScene;
        }
        else if (mSceneState == SceneState::Simulate)
        {
            mEditorScene->SetSelectedEntity(entity);
            mCurrentScene = mSimulationScene;
        }
    }

    void EditorLayer::OnEvent(Event& e)
    {
        if (mSceneState == SceneState::Edit)
        {
            if (mViewportPanelMouseOver)
            {
                mEditorCamera.OnEvent(e);
            }

            mEditorScene->OnEvent(e);
        }
        else if (mSceneState == SceneState::Simulate)
        {
            if (mViewportPanelMouseOver)
            {
                mEditorCamera.OnEvent(e);
            }
        }
        else if (mSceneState == SceneState::Play)
        {
            mRuntimeScene->OnEvent(e);
        }

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>(NR_BIND_EVENT_FN(EditorLayer::OnKeyPressedEvent));
        dispatcher.Dispatch<MouseButtonPressedEvent>(NR_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
    }

    bool EditorLayer::OnKeyPressedEvent(KeyPressedEvent& e)
    {
        if (GImGui->ActiveId == 0)
        {
            if (mViewportPanelMouseOver)
            {
                switch (e.GetKeyCode())
                {
                case KeyCode::Q:
                {
                    mGizmoType = -1;
                    break;
                }
                case KeyCode::W:
                {
                    mGizmoType = ImGuizmo::OPERATION::TRANSLATE;
                    break;
                }
                case KeyCode::E:
                {
                    mGizmoType = ImGuizmo::OPERATION::ROTATE;
                    break;
                }
                case KeyCode::R:
                {
                    mGizmoType = ImGuizmo::OPERATION::SCALE;
                    break;
                }
                case KeyCode::F:
                {
                    if (mSelectionContext.size() == 0)
                    {
                        break;
                    }

                    Entity selectedEntity = mSelectionContext[0].EntityObj;
                    mEditorCamera.Focus(selectedEntity.Transform().Translation);
                    break;
                }
                }

            }
            switch (e.GetKeyCode())
            {
            case KeyCode::Escape:
                if (mSelectionContext.size())
                {
                    mSelectionContext.clear();
                    mEditorScene->SetSelectedEntity({});
                    mSceneHierarchyPanel->SetSelected({});
                }
                break;
            case KeyCode::Delete:
                if (mSelectionContext.size())
                {
                    DeleteEntity(mSelectionContext[0].EntityObj);
                    mSelectionContext.clear();
                    mEditorScene->SetSelectedEntity({});
                    mSceneHierarchyPanel->SetSelected({});
                }
                break;
            }
        }

        if (Input::IsKeyPressed(NR_KEY_LEFT_CONTROL))
        {
            switch (e.GetKeyCode())
            {
            case KeyCode::B:
            {
                mUIShowBoundingBoxes = !mUIShowBoundingBoxes;
                ShowBoundingBoxes(mUIShowBoundingBoxes, mUIShowBoundingBoxesOnTop);
                break;
            }
            case KeyCode::D:
            {
                if (mSelectionContext.size())
                {
                    Entity selectedEntity = mSelectionContext[0].EntityObj;
                    mEditorScene->DuplicateEntity(selectedEntity);
                }
                break;
            }
            case KeyCode::G:
            {
                SceneRenderer::GetOptions().ShowGrid = !SceneRenderer::GetOptions().ShowGrid;
                break;
            }
            case KeyCode::N:
            {
                NewScene();
                break;
            }
            case KeyCode::O:
            {
                OpenScene();
                break;
            }
            case KeyCode::S:
            {
                SaveScene();
                break;
            }
            }

            if (Input::IsKeyPressed(NR_KEY_LEFT_SHIFT))
            {
                switch (e.GetKeyCode())
                {
                case KeyCode::S:
                {
                    SaveSceneAs();
                    break;
                }
                }
            }
        }

        return false;
    }

    bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
    {
        auto [mx, my] = Input::GetMousePosition();
        if (e.GetMouseButton() == NR_MOUSE_BUTTON_LEFT && mViewportPanelMouseOver && !Input::IsKeyPressed(KeyCode::LeftAlt) && !ImGuizmo::IsOver() && mSceneState != SceneState::Play)
        {
            auto [mouseX, mouseY] = GetMouseViewportSpace();
            if (mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f)
            {
                auto [origin, direction] = CastRay(mouseX, mouseY);

                mSelectionContext.clear();
                mCurrentScene->SetSelectedEntity({});
                auto meshEntities = mCurrentScene->GetAllEntitiesWith<MeshComponent>();
                for (auto e : meshEntities)
                {
                    Entity entity = { e, mCurrentScene.Raw() };
                    auto mesh = entity.GetComponent<MeshComponent>().MeshObj;
                    if (!mesh || mesh->Type == AssetType::Missing)
                    {
                        continue;
                    }

                    auto& submeshes = mesh->GetSubmeshes();
                    constexpr float lastT = std::numeric_limits<float>::max();
                    for (uint32_t i = 0; i < submeshes.size(); ++i)
                    {
                        auto& submesh = submeshes[i];
                        glm::mat4 transform = mCurrentScene->GetTransformRelativeToParent(entity);
                        Ray ray = {
                            glm::inverse(transform * submesh.Transform) * glm::vec4(origin, 1.0f),
                            glm::inverse(glm::mat3(transform) * glm::mat3(submesh.Transform)) * direction
                        };

                        float t;
                        bool intersects = ray.IntersectsAABB(submesh.BoundingBox, t);
                        if (intersects)
                        {
                            const auto& triangleCache = mesh->GetTriangleCache(i);
                            for (const auto& triangle : triangleCache)
                            {
                                if (ray.IntersectsTriangle(triangle.V0.Position, triangle.V1.Position, triangle.V2.Position, t))
                                {
                                    NR_WARN("INTERSECTION: {0}, t = {1}", submesh.NodeName, t);
                                    mSelectionContext.push_back({ entity, &submesh, t });
                                    break;
                                }
                            }
                        }
                    }
                }

                std::sort(mSelectionContext.begin(), mSelectionContext.end(), [](auto& a, auto& b) { return a.Distance < b.Distance; });
                if (mSelectionContext.size())
                {
                    Selected(mSelectionContext[0]);
                }
            }
        }
        return false;
    }

    std::pair<float, float> EditorLayer::GetMouseViewportSpace()
    {
        auto [mx, my] = ImGui::GetMousePos();
        mx -= mViewportBounds[0].x;
        my -= mViewportBounds[0].y;
        auto viewportWidth = mViewportBounds[1].x - mViewportBounds[0].x;
        auto viewportHeight = mViewportBounds[1].y - mViewportBounds[0].y;

        return { (mx / viewportWidth) * 2.0f - 1.0f, ((my / viewportHeight) * 2.0f - 1.0f) * -1.0f };
    }

    std::pair<glm::vec3, glm::vec3> EditorLayer::CastRay(float mx, float my)
    {
        glm::vec4 mouseClipPos = { mx, my, -1.0f, 1.0f };

        auto inverseProj = glm::inverse(mEditorCamera.GetProjectionMatrix());
        auto inverseView = glm::inverse(glm::mat3(mEditorCamera.GetViewMatrix()));

        glm::vec4 ray = inverseProj * mouseClipPos;
        glm::vec3 rayPos = mEditorCamera.GetPosition();
        glm::vec3 rayDir = inverseView * glm::vec3(ray);

        return { rayPos, rayDir };
    }

    void EditorLayer::Selected(const SelectedSubmesh& selectionContext)
    {
        mSceneHierarchyPanel->SetSelected(selectionContext.EntityObj);
        mCurrentScene->SetSelectedEntity(selectionContext.EntityObj);
    }

    void EditorLayer::EntityDeleted(Entity e)
    {
        if (mSelectionContext.size() > 0 && mSelectionContext[0].EntityObj == e)
        {
            mSelectionContext.clear();
            mEditorScene->SetSelectedEntity({});
        }
    }

    Ray EditorLayer::CastMouseRay()
    {
        auto [mouseX, mouseY] = GetMouseViewportSpace();
        if (mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f)
        {
            auto [origin, direction] = CastRay(mouseX, mouseY);
            return Ray(origin, direction);
        }
        return Ray::Zero();
    }
}