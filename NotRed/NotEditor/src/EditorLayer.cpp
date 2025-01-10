#include "EditorLayer.h"

#include <filesystem>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui_internal.h"
#include "NotRed/ImGui/ImGui.h"

#include "NotRed/Scene/Prefab.h"

#include "NotRed/ImGui/ImGuizmo.h"
#include "NotRed/Renderer/Renderer2D.h"
#include "NotRed/Script/ScriptEngine.h"
#include "NotRed/Editor/AssetEditorPanel.h"
#include "NotRed/Audio/Editor/AudioEventsEditor.h"

#include "NotRed/Audio/AudioEvents/AudioCommandRegistry.h"

#include "NotRed/Physics/PhysicsManager.h"
#include "NotRed/Physics/PhysicsLayer.h"
#include "NotRed/Physics/Debug/PhysicsDebugger.h"

#include "NotRed/Math/Math.h"
#include "NotRed/Util/FileSystem.h"

#include "NotRed/Project/Project.h"
#include "NotRed/Project/ProjectSerializer.h"

#include "NotRed/Audio/AudioEngine.h"
#include "NotRed/Audio/SceneAudio.h"

#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
#define MAX_PROJECT_NAME_LENGTH 255
#define MAX_PROJECT_FILEPATH_LENGTH 512

    static char* sProjectNameBuffer = new char[MAX_PROJECT_NAME_LENGTH];
    static char* sProjectFilePathBuffer = new char[MAX_PROJECT_FILEPATH_LENGTH];
    EditorLayer::EditorLayer(const Ref<UserPreferences>& userPreferences)
        : mUserPreferences(userPreferences)
    {
    }

    EditorLayer::~EditorLayer()
    {
    }

    auto operator < (const ImVec2& lhs, const ImVec2& rhs)
    {
        return lhs.x < rhs.x && lhs.y < rhs.y;
    }

    static std::string sNotRedInstallPath = "";

    void EditorLayer::Attach()
    {
        using namespace glm;

        // Editor
        mCheckerboardTex = Texture2D::Create("Resources/Editor/Checkerboard.tga");
        mPlayButtonTex = Texture2D::Create("Resources/Editor/PlayButton.png");
        mPauseButtonTex = Texture2D::Create("Resources/Editor/PauseButton.png");
        mStopButtonTex = Texture2D::Create("Resources/Editor/StopButton.png");
        mSelectToolTex = Texture2D::Create("Resources/Editor/SelectTool.png");
        mMoveToolTex = Texture2D::Create("Resources/Editor/MoveTool.png");
        mRotateToolTex = Texture2D::Create("Resources/Editor/RotateTool.png");
        mScaleToolTex = Texture2D::Create("Resources/Editor/ScaleTool.png");

        mPointLightIcon = Texture2D::Create("Resources/Editor/Icons/PointLight.png");

        mSceneHierarchyPanel = CreateScope<SceneHierarchyPanel>(mEditorScene);
        mSceneHierarchyPanel->SetSelectionChangedCallback(std::bind(&EditorLayer::SelectEntity, this, std::placeholders::_1));
        mSceneHierarchyPanel->SetEntityDeletedCallback(std::bind(&EditorLayer::EntityDeleted, this, std::placeholders::_1));
        mSceneHierarchyPanel->SetMeshAssetConvertCallback(std::bind(&EditorLayer::CreateMeshFromMeshAsset, this, std::placeholders::_1, std::placeholders::_2));
        mSceneHierarchyPanel->SetInvalidMetadataCallback(std::bind(&EditorLayer::SceneHierarchyInvalidMetadataCallback, this, std::placeholders::_1, std::placeholders::_2));

        mPrefabHierarchyPanel = CreateScope<SceneHierarchyPanel>();

        Renderer2D::SetLineWidth(mLineWidth);

        if (!mUserPreferences->StartupProject.empty())
        {
            OpenProject(mUserPreferences->StartupProject);
        }

        mObjectsPanel = CreateScope<ObjectsPanel>();
        mConsolePanel = CreateScope<EditorConsolePanel>();

        mViewportRenderer = Ref<SceneRenderer>::Create(mCurrentScene);
        mSecondViewportRenderer = Ref<SceneRenderer>::Create(mCurrentScene);
        mFocusedRenderer = mViewportRenderer;

        AssetEditorPanel::RegisterDefaultEditors();
        //FileSystem::StartWatching();

        Renderer2D::SetLineWidth(mLineWidth);
        mViewportRenderer->SetLineWidth(mLineWidth);

        UpdateSceneRendererSettings();
        AudioEventsEditor::Init();

        sNotRedInstallPath = FileSystem::GetEnvironmentVariable("NOTRED_DIR");
    }

    void EditorLayer::Detach()
    {
        CloseProject(false);
        AudioEventsEditor::Shutdown();
        FileSystem::StopWatching();
        AssetEditorPanel::UnregisterAllEditors();
    }

    void EditorLayer::ScenePlay()
    {
        mSelectionContext.clear();

        mSceneState = SceneState::Play;
        UI::SetMouseEnabled(true);
        Input::SetCursorMode(CursorMode::Normal);

        mConsolePanel->ScenePlay();

        if (Project::GetActive()->GetConfig().ReloadAssemblyOnPlay)
        {
            ScriptEngine::ReloadAssembly((Project::GetScriptModuleFilePath()).string());
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
        Input::SetCursorMode(CursorMode::Normal);
        UI::SetMouseEnabled(true);

        mRuntimeScene = nullptr;

        mSelectionContext.clear();
        ScriptEngine::SetSceneContext(mEditorScene);
        AudioEngine::SetSceneContext(mEditorScene);
        mSceneHierarchyPanel->SetContext(mEditorScene);
        mCurrentScene = mEditorScene;
    }

    void EditorLayer::UpdateWindowTitle(const std::string& sceneName)
    {
        std::string rendererAPI = RendererAPI::Current() == RendererAPIType::Vulkan ? "Vulkan" : "OpenGL";
        std::string title = fmt::format("{0} ({1}) - NotEditor - {2} ({3}) Renderer: {4}", sceneName, Project::GetActive()->GetConfig().Name, Application::GetPlatformName(), Application::GetConfigurationName(), rendererAPI);
        Application::Get().GetWindow().SetTitle(title);
    }

    void EditorLayer::UpdateSceneRendererSettings()
    {
        std::array<Ref<SceneRenderer>, 2> renderers = { mViewportRenderer, mSecondViewportRenderer };
        for (Ref<SceneRenderer> renderer : renderers)
        {
            SceneRendererOptions& options = renderer->GetOptions();
            options.ShowSelectedInWireframe = mShowSelectedWireframe;

            SceneRendererOptions::PhysicsColliderView colliderView = SceneRendererOptions::PhysicsColliderView::None;
            if (mShowPhysicsCollidersWireframe)
            {
                colliderView = mShowPhysicsCollidersWireframeOnTop ? SceneRendererOptions::PhysicsColliderView::OnTop : SceneRendererOptions::PhysicsColliderView::Normal;
            }
            options.ShowPhysicsColliders = colliderView;
        }
    }

    float EditorLayer::GetSnapValue()
    {
        switch (mGizmoType)
        {
        case  ImGuizmo::OPERATION::TRANSLATE: return 0.5f;
        case  ImGuizmo::OPERATION::ROTATE: return 45.0f;
        case  ImGuizmo::OPERATION::SCALE: return 0.5f;
        default: return 0.0f;
        }
    }

    void EditorLayer::DeleteEntity(Entity entity)
    {
        auto children = entity.Children();
        for (auto childId : children)
        {
            DeleteEntity(mEditorScene->FindEntityByID(childId));
        }

        mEditorScene->UnparentEntity(entity);
        mEditorScene->DestroyEntity(entity);
    }

    void EditorLayer::Update(float dt)
    {
        switch (mSceneState)
        {
        case SceneState::Edit:
        {
            mEditorCamera.SetActive(mViewportHasCapturedMouse || Input::GetCursorMode() == CursorMode::Locked);
            mEditorCamera.Update(dt);
            mEditorScene->RenderEditor(mViewportRenderer, dt, mEditorCamera);

            if (mShowSecondViewport)
            {
                mSecondEditorCamera.SetActive(mViewportPanel2Focused);
                mSecondEditorCamera.Update(dt);
                mEditorScene->RenderEditor(mSecondViewportRenderer, dt, mSecondEditorCamera);
            }

            Render2D();
            break;
        }
        case SceneState::Play:
        {
            mEditorCamera.SetActive(false);
            UI::SetMouseEnabled(true);

            mRuntimeScene->Update(dt);
            mRuntimeScene->RenderRuntime(mViewportRenderer, dt);
            break;
        }
        case SceneState::Pause:
        {
            mEditorCamera.SetActive(true);
            UI::SetMouseEnabled(true);

            mEditorCamera.Update(dt);
            mRuntimeScene->RenderRuntime(mViewportRenderer, dt);
            break;
        }
        }
        AssetEditorPanel::Update(dt);
        SceneRenderer::WaitForThreads();
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

            if (meshComp.MeshObj && !meshComp.MeshObj->IsFlagSet(AssetFlag::Missing))
            {
                selection.Mesh = &meshComp.MeshObj->GetMeshAsset()->GetSubmeshes()[0];
            }
        }

        selection.EntityObj = entity;

        mSelectionContext.clear();
        mSelectionContext.push_back(selection);
        mEditorScene->SetSelectedEntity(entity);
        mCurrentScene = mEditorScene;
    }

    void EditorLayer::Render2D()
    {
        if (!mViewportRenderer->GetFinalPassImage())
        {
            return;
        }

        Renderer2D::BeginScene(mEditorCamera.GetViewProjection(), mEditorCamera.GetViewMatrix());
        Renderer2D::SetTargetRenderPass(mViewportRenderer->GetExternalCompositeRenderPass());

        if (mDrawOnTopBoundingBoxes)
        {
            if (mShowBoundingBoxes)
            {
                if (mShowBoundingBoxSelectedMeshOnly)
                {
                    if (mSelectionContext.size())
                    {
                        auto& selection = mSelectionContext[0];
                        if (selection.EntityObj.HasComponent<MeshComponent>())
                        {
                            if (mShowBoundingBoxSubmeshes)
                            {
                                auto& mesh = selection.EntityObj.GetComponent<MeshComponent>().MeshObj;
                                if (mesh)
                                {
                                    auto& submeshIndices = mesh->GetSubmeshes();
                                    auto meshAsset = mesh->GetMeshAsset();
                                    auto& submeshes = meshAsset->GetSubmeshes();
                                    for (uint32_t submeshIndex : submeshIndices)
                                    {
                                        glm::mat4 transform = selection.EntityObj.GetComponent<TransformComponent>().GetTransform();
                                        const AABB& aabb = submeshes[submeshIndex].BoundingBox;
                                        Renderer2D::DrawAABB(aabb, transform * submeshes[submeshIndex].Transform, glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f });
                                    }
                                }
                            }
                            else
                            {
                                auto& mesh = selection.EntityObj.GetComponent<MeshComponent>().MeshObj;
                                if (mesh)
                                {
                                    glm::mat4 transform = selection.EntityObj.GetComponent<TransformComponent>().GetTransform();
                                    const AABB& aabb = mesh->GetMeshAsset()->GetBoundingBox();
                                    Renderer2D::DrawAABB(aabb, transform, glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f });
                                }
                            }
                        }
                    }
                }
                else
                {
                    auto entities = mCurrentScene->GetAllEntitiesWith<MeshComponent>();
                    for (auto e : entities)
                    {
                        Entity entity = { e, mCurrentScene.Raw() };
                        glm::mat4 transform = entity.GetComponent<TransformComponent>().GetTransform();
                        const AABB& aabb = entity.GetComponent<MeshComponent>().MeshObj->GetMeshAsset()->GetBoundingBox();
                        Renderer2D::DrawAABB(aabb, transform, glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f });
                    }
                }
            }


#if RENDERER_2D
            if (mDrawOnTopBoundingBoxes)
            {
                Renderer::BeginRenderPass(mViewportRenderer->GetFinalRenderPass(), false);
                auto viewProj = mEditorCamera.GetViewProjection();
                Renderer2D::BeginScene(viewProj, false);
                glm::vec4 color = (mSelectionMode == SelectionMode::Entity) ? glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f } : glm::vec4{ 0.2f, 0.9f, 0.2f, 1.0f };
                Renderer::DrawAABB(selection.Mesh->BoundingBox, selection.Entity.Transform().GetTransform() * selection.Mesh->Transform, color);
                Renderer2D::EndScene();
                Renderer::EndRenderPass();
            }
        }
        if (mSelectionContext.size())
        {
            auto& selection = mSelectionContext[0];
            if (selection.Entity.HasComponent<BoxCollider2DComponent>() && false)
            {
                const auto& size = selection.Entity.GetComponent<BoxCollider2DComponent>().Size;
                const TransformComponent& transform = selection.Entity.GetComponent<TransformComponent>();
                if (selection.Mesh && selection.Entity.HasComponent<MeshComponent>())
                {
                    Renderer::BeginRenderPass(mViewportRenderer->GetFinalRenderPass(), false);
                    auto viewProj = mEditorCamera.GetViewProjection();
                    Renderer2D::BeginScene(viewProj, false);
                    glm::vec4 color = (mSelectionMode == SelectionMode::Entity) ? glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f } : glm::vec4{ 0.2f, 0.9f, 0.2f, 1.0f };
                    Renderer::DrawAABB(selection.Mesh->BoundingBox, selection.Entity.Transform().GetTransform() * selection.Mesh->Transform, color);
                    Renderer2D::EndScene();
                    Renderer::EndRenderPass();
                }
            }

            if (selection.Entity.HasComponent<CircleCollider2DComponent>())
            {
                auto& selection = mSelectionContext[0];
                if (selection.Entity.HasComponent<BoxCollider2DComponent>() && false)
                {
                    const auto& size = selection.Entity.GetComponent<BoxCollider2DComponent>().Size;
                    const TransformComponent& transform = selection.Entity.GetComponent<TransformComponent>();

                    Renderer::BeginRenderPass(mViewportRenderer->GetFinalRenderPass(), false);
                    auto viewProj = mEditorCamera.GetViewProjection();
                    Renderer2D::BeginScene(viewProj, false);
                    Renderer2D::DrawRotatedRect({ transform.Translation.x, transform.Translation.y }, size * 2.0f, transform.Rotation.z, { 0.0f, 1.0f, 1.0f, 1.0f });
                    Renderer2D::EndScene();
                    Renderer::EndRenderPass();
                }

                if (selection.Entity.HasComponent<CircleCollider2DComponent>() && false)
                {
                    const auto& size = selection.Entity.GetComponent<CircleCollider2DComponent>().Radius;
                    const TransformComponent& transform = selection.Entity.GetComponent<TransformComponent>();

                    Renderer::BeginRenderPass(mViewportRenderer->GetFinalRenderPass(), false);
                    auto viewProj = mEditorCamera.GetViewProjection();
                    Renderer2D::BeginScene(viewProj, false);
                    Renderer2D::DrawCircle({ transform.Translation.x, transform.Translation.y }, size, { 0.0f, 1.0f, 1.0f, 1.0f });
                    Renderer2D::EndScene();
                    Renderer::EndRenderPass();
                }

                Renderer::BeginRenderPass(SceneRenderer::GetFinalRenderPass(), false);
                auto viewProj = mEditorCamera.GetViewProjection();
                Renderer2D::BeginScene(viewProj, false);
                Renderer2D::DrawCircle({ transform.Translation.x, transform.Translation.y }, size, { 0.0f, 1.0f, 1.0f, 1.0f });
                Renderer2D::EndScene();
                Renderer::EndRenderPass();
            }
#endif
        }

        if (mShowIcons)
        {
            auto entities = mCurrentScene->GetAllEntitiesWith<PointLightComponent>();
            for (auto e : entities)
            {
                Entity entity = { e, mCurrentScene.Raw() };
                auto& tc = entity.GetComponent<TransformComponent>();
                auto& plc = entity.GetComponent<PointLightComponent>();
                Renderer2D::DrawQuadBillboard(tc.Translation, { 1.0f, 1.0f }, mPointLightIcon);
            }
        }

        if (mSelectionContext.size())
        {
            auto& selection = mSelectionContext[0];
            if (selection.EntityObj.HasComponent<PointLightComponent>())
            {
                auto& tc = selection.EntityObj.GetComponent<TransformComponent>();
                auto& plc = selection.EntityObj.GetComponent<PointLightComponent>();
                Renderer2D::DrawCircle(tc.Translation, { 0.0f, 0.0f, 0.0f }, plc.Radius, glm::vec4(plc.Radiance, 1.0f));
                Renderer2D::DrawCircle(tc.Translation, { glm::radians(90.0f), 0.0f, 0.0f }, plc.Radius, glm::vec4(plc.Radiance, 1.0f));
                Renderer2D::DrawCircle(tc.Translation, { 0.0f, glm::radians(90.0f), 0.0f }, plc.Radius, glm::vec4(plc.Radiance, 1.0f));
            }
        }

        Renderer2D::EndScene();
    }

    static void ReplaceProjectName(std::string& str, const std::string& projectName)
    {
        static const char* sProjectNameToken = "$PROJECT_NAME$";
        size_t pos = 0;
        while ((pos = str.find(sProjectNameToken, pos)) != std::string::npos)
        {
            str.replace(pos, strlen(sProjectNameToken), projectName);
            pos += strlen(sProjectNameToken);
        }
    }

    void EditorLayer::CreateProject(std::filesystem::path projectPath)
    {
        if (!FileSystem::HasEnvironmentVariable("NOTRED_DIR"))
        {
            return;
        }

        if (!std::filesystem::exists(projectPath))
        {
            std::filesystem::create_directories(projectPath);
        }

        std::filesystem::copy(sNotRedInstallPath + "/NotEditor/Resources/NewProjectTemplate", projectPath, std::filesystem::copy_options::recursive);
        {
            // premake5.lua
            std::ifstream stream(projectPath / "premake5.lua");
            NR_CORE_VERIFY(stream.is_open());

            std::stringstream ss;
            ss << stream.rdbuf();
            stream.close();
            std::string str = ss.str();

            ReplaceProjectName(str, sProjectNameBuffer);

            std::ofstream ostream(projectPath / "premake5.lua");
            ostream << str;
            ostream.close();
        }
        {
            // Project File
            std::ifstream stream(projectPath / "Project.nrproj");
            NR_CORE_VERIFY(stream.is_open());

            std::stringstream ss;
            ss << stream.rdbuf();
            stream.close();
            std::string str = ss.str();

            ReplaceProjectName(str, sProjectNameBuffer);

            std::ofstream ostream(projectPath / "Project.nrproj");
            ostream << str;
            ostream.close();

            std::string newProjectFileName = std::string(sProjectNameBuffer) + ".nrproj";
            std::filesystem::rename(projectPath / "Project.nrproj", projectPath / newProjectFileName);
        }

        std::filesystem::create_directories(projectPath / "Assets" / "Audio");
        std::filesystem::create_directories(projectPath / "Assets" / "Materials");
        std::filesystem::create_directories(projectPath / "Assets" / "Meshes" / "Source");
        std::filesystem::create_directories(projectPath / "Assets" / "Scenes");
        std::filesystem::create_directories(projectPath / "Assets" / "Scripts" / "Source");
        std::filesystem::create_directories(projectPath / "Assets" / "Textures");

        std::string batchFilePath = projectPath.string();
        std::replace(batchFilePath.begin(), batchFilePath.end(), '/', '\\'); // Only windows
        batchFilePath += "\\Win-CreateScriptProjects.bat";
        system(batchFilePath.c_str());
        OpenProject(projectPath.string() + "/" + std::string(sProjectNameBuffer) + ".nrproj");
    }

    void EditorLayer::OpenProject()
    {
        auto& app = Application::Get();
        std::string filepath = app.OpenFile("NotRed Project (*.nrproj)\0*.nrproj\0");

        if (filepath.empty())
        {
            return;
        }

        OpenProject(filepath);
        std::filesystem::path projectFile = filepath;

        RecentProject projectEntry;
        projectEntry.Name = Utils::RemoveExtension(projectFile.filename().string());
        projectEntry.FilePath = filepath;
        projectEntry.LastOpened = time(NULL);

        for (auto it = mUserPreferences->RecentProjects.begin(); it != mUserPreferences->RecentProjects.end(); it++)
        {
            if (it->second.Name == projectEntry.Name)
            {
                mUserPreferences->RecentProjects.erase(it);
                break;
            }
        }

        mUserPreferences->RecentProjects[projectEntry.LastOpened] = projectEntry;
        UserPreferencesSerializer serializer(mUserPreferences);
        serializer.Serialize(mUserPreferences->FilePath);
    }

    void EditorLayer::OpenProject(const std::string& filepath)
    {
        if (Project::GetActive())
        {
            CloseProject();
        }

        Ref<Project> project = Ref<Project>::Create();
        ProjectSerializer serializer(project);

        serializer.Deserialize(filepath);

        Project::SetActive(project);
        ScriptEngine::LoadAppAssembly((Project::GetScriptModuleFilePath()).string());

        if (!project->GetConfig().StartScene.empty())
        {
            OpenScene((Project::GetAssetDirectory() / project->GetConfig().StartScene).string());
        }
        else
        {
            NewScene();
        }

        if (mEditorScene)
        {
            mEditorScene->SetSelectedEntity({});
        }

        mSelectionContext.clear();
        mContentBrowserPanel = CreateScope<ContentBrowserPanel>(project);
        mProjectSettingsPanel = CreateScope<ProjectSettingsWindow>(project);
        FileSystem::StartWatching();
        
        // Reset cameras
        mEditorCamera = EditorCamera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f));
        mSecondEditorCamera = EditorCamera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f));
    }

    void EditorLayer::SaveProject()
    {
        if (!Project::GetActive())
        {
            NR_CORE_ASSERT(false);
        }

        auto project = Project::GetActive();
        ProjectSerializer serializer(project);
        serializer.Serialize(project->GetConfig().ProjectDirectory + "/" + project->GetConfig().ProjectFileName);
    }

    void EditorLayer::CloseProject(bool unloadProject)
    {
        FileSystem::StopWatching();

        SaveProject();
        
        mSceneHierarchyPanel->SetContext(nullptr);
        ScriptEngine::SetSceneContext(nullptr);
        AudioEngine::SetSceneContext(nullptr);
        mViewportRenderer->SetScene(nullptr);
        mSecondViewportRenderer->SetScene(nullptr);

        mRuntimeScene = nullptr;
        mCurrentScene = nullptr;

        NR_CORE_ASSERT(mEditorScene->GetRefCount() == 1, "Scene will not be destroyed after project is closed - something is still holding scene refs!");
        mEditorScene = nullptr;

        PhysicsLayerManager::ClearLayers();

        if (unloadProject)
        {
            Project::SetActive(nullptr);
        }
    }

    void EditorLayer::NewScene()
    {
        mSelectionContext = {};

        mEditorScene = Ref<Scene>::Create("Scene", true);
        mSceneHierarchyPanel->SetContext(mEditorScene);
        ScriptEngine::SetSceneContext(mEditorScene);
        AudioEngine::SetSceneContext(mEditorScene);
        UpdateWindowTitle("Scene");
        mSceneFilePath = std::string();

        mEditorCamera = EditorCamera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f));
        mCurrentScene = mEditorScene;

        if (mViewportRenderer)
        {
            mViewportRenderer->SetScene(mCurrentScene);
        }
        if (mSecondViewportRenderer)
        {
            mSecondViewportRenderer->SetScene(mCurrentScene);
        }
    }

    void EditorLayer::OpenScene()
    {
        auto& app = Application::Get();
        std::string filepath = app.OpenFile(SceneSerializer::FileFilter.data());
        if (!filepath.empty())
        {
            OpenScene(filepath);
        }
    }

    void EditorLayer::OpenScene(const std::string& filepath)
    {
        Ref<Scene> newScene = Ref<Scene>::Create("New Scene", true);
        SceneSerializer serializer(newScene);
        serializer.Deserialize(filepath);
        mEditorScene = newScene;
        mSceneFilePath = filepath;

        std::filesystem::path path = filepath;
        UpdateWindowTitle(path.filename().string());
        mSceneHierarchyPanel->SetContext(mEditorScene);
        ScriptEngine::SetSceneContext(mEditorScene);
        AudioEngine::SetSceneContext(mEditorScene);

        mEditorScene->SetSelectedEntity({});
        mSelectionContext.clear();

        mCurrentScene = mEditorScene;
    }

    void EditorLayer::OpenScene(const AssetMetadata& assetMetadata)
    {
        std::filesystem::path workingDirPath = Project::GetAssetDirectory() / assetMetadata.FilePath;
        OpenScene(workingDirPath.string());
    }

    void EditorLayer::SaveScene()
    {
        if (!mSceneFilePath.empty())
        {
            SceneSerializer serializer(mEditorScene);
            serializer.Serialize(mSceneFilePath);

            AudioCommandRegistry::WriteRegistryToFile();
        }
        else
        {
            SaveSceneAs();
        }
    }

    void EditorLayer::SaveSceneAs()
    {
        auto& app = Application::Get();
        std::filesystem::path filepath = app.SaveFile(SceneSerializer::FileFilter.data());
        if (!filepath.empty())
        {
            if (!filepath.has_extension())
            {
                filepath += SceneSerializer::DefaultExtension;
            }

            SceneSerializer serializer(mEditorScene);
            serializer.Serialize(filepath.string());

            std::filesystem::path path = filepath;
            UpdateWindowTitle(path.filename().string());
            mSceneFilePath = filepath.string();
        }
    }

    void EditorLayer::UI_WelcomePopup()
    {
        if (mUserPreferences->ShowWelcomeScreen && mShowWelcomePopup)
        {
            ImGui::OpenPopup("Welcome");
            mShowWelcomePopup = false;
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2{ 400,0 });

        if (ImGui::BeginPopupModal("Welcome", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Welcome to NotRed!");
            ImGui::Separator();
            ImGui::TextWrapped("NotRed is an early-stage interactive application and rendering engine for Windows.");

            if (ImGui::Button("OK"))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine(250.0f);
            ImGui::TextUnformatted("Don't Show Again");
            ImGui::SameLine(365.0f);
            
            bool dontShowAgain = !mUserPreferences->ShowWelcomeScreen;
            if (ImGui::Checkbox("##dont_show_again", &dontShowAgain))
            {
                mUserPreferences->ShowWelcomeScreen = !dontShowAgain;
                UserPreferencesSerializer serializer(mUserPreferences);
                serializer.Serialize(mUserPreferences->FilePath);
            }

            ImGui::EndPopup();
        }
    }

    void EditorLayer::UI_AboutPopup()
    {
        ImGuiIO& io = ImGui::GetIO();

        auto boldFont = io.Fonts->Fonts[0];
        auto largeFont = io.Fonts->Fonts[1];

        if (mShowAboutPopup)
        {
            ImGui::OpenPopup("About##AboutPopup");
            mShowAboutPopup = false;
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2{ 600,0 });

        if (ImGui::BeginPopupModal("About##AboutPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::PushFont(largeFont);
            ImGui::Text("NotRed Engine");

            ImGui::PopFont();
            ImGui::Separator();

            ImGui::TextWrapped("NotRed is an early-stage interactive application and rendering engine for Windows.");

            ImGui::Separator();
            ImGui::PushFont(boldFont);

            ImGui::Text("NotRed Core Team");
            ImGui::PopFont();

            ImGui::Text("Anselmo Noriega");
            ImGui::Text("Special Thanks To: The Cherno Team");

            ImGui::Separator();
            ImGui::TextColored(ImVec4{ 0.7f, 0.7f, 0.7f, 1.0f }, "This software contains source code provided by NVIDIA Corporation.");

            if (ImGui::Button("OK"))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void EditorLayer::UI_CreateNewMeshPopup()
    {
        if (mShowCreateNewMeshPopup)
        {
            ImGui::OpenPopup("Create New Mesh");
            mShowCreateNewMeshPopup = false;
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2{ 400,0 });

        if (ImGui::BeginPopupModal("Create New Mesh", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextWrapped("A Mesh asset must be created from this Mesh Source file (eg. FBX) before it can be added to your scene. More options can be accessed by double-clicking a Mesh Source file in the Content Browser panel.");

            const std::string& meshAssetPath = Project::GetActive()->GetConfig().MeshPath;

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Path: %s/", meshAssetPath.c_str());
            ImGui::SameLine();

            NR_CORE_ASSERT(mCreateNewMeshPopupData.MeshToCreate);
            const AssetMetadata& assetData = AssetManager::GetMetadata(mCreateNewMeshPopupData.MeshToCreate->Handle);
            std::string filepath = fmt::format("{0}/{1}.nrmesh", mCurrentScene->GetName(), assetData.FilePath.stem().string());

            if (!mCreateNewMeshPopupData.CreateMeshFilenameBuffer[0])
            {
                strcpy(mCreateNewMeshPopupData.CreateMeshFilenameBuffer.data(), filepath.c_str());
            }

            ImGui::InputText("##meshPath", mCreateNewMeshPopupData.CreateMeshFilenameBuffer.data(), 256);

            if (ImGui::Button("Create"))
            {
                std::string serializePath = mCreateNewMeshPopupData.CreateMeshFilenameBuffer.data();
                std::filesystem::path path = Project::GetActive()->GetMeshPath() / serializePath;

                if (!FileSystem::Exists(path.parent_path()))
                {
                    FileSystem::CreateDirectory(path.parent_path());
                }

                Ref<Mesh> mesh = AssetManager::CreateNewAsset<Mesh>(path.filename().string(), path.parent_path().string(), mCreateNewMeshPopupData.MeshToCreate);
                Entity entity = mCreateNewMeshPopupData.TargetEntity;
                if (entity)
                {
                    if (!entity.HasComponent<MeshComponent>())
                    {
                        entity.AddComponent<MeshComponent>();
                    }
                    MeshComponent& mc = entity.GetComponent<MeshComponent>();
                    mc.MeshObj = mesh;
                }
                else
                {
                    const auto& meshMetadata = AssetManager::GetMetadata(mesh->Handle);
                    Entity entity = mEditorScene->CreateEntity(meshMetadata.FilePath.stem().string());
                    entity.AddComponent<MeshComponent>(mesh);
                    SelectEntity(entity);
                }
                mCreateNewMeshPopupData = {};
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel"))
            {
                mCreateNewMeshPopupData = {};
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void EditorLayer::UI_InvalidAssetMetadataPopup()
    {
        if (mShowInvalidAssetMetadataPopup)
        {
            ImGui::OpenPopup("Invalid Asset Metadata");
            mShowInvalidAssetMetadataPopup = false;
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2{ 400,0 });

        if (ImGui::BeginPopupModal("Invalid Asset Metadata", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextWrapped("You tried to use an asset with invalid metadata. This can happen when an asset has a reference to another non-existent asset, or when as asset is empty.");

            ImGui::Separator();

            auto& metadata = mInvalidAssetMetadataPopupData.Metadata;

            UI::BeginPropertyGrid();

            const auto& filepath = metadata.FilePath.string();
            UI::Property("Asset Filepath", filepath);
            UI::Property("Asset ID", fmt::format("{0}", (uint64_t)metadata.Handle));
            UI::EndPropertyGrid();
            if (ImGui::Button("OK"))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
    void EditorLayer::CreateMeshFromMeshAsset(Entity entity, Ref<MeshAsset> meshAsset)
    {
        mShowCreateNewMeshPopup = true;
        mCreateNewMeshPopupData.MeshToCreate = meshAsset;
        mCreateNewMeshPopupData.TargetEntity = entity;
    }

    void EditorLayer::SceneHierarchyInvalidMetadataCallback(Entity entity, AssetHandle handle)
    {
        mShowInvalidAssetMetadataPopup = true;
        mInvalidAssetMetadataPopupData.Metadata = AssetManager::GetMetadata(handle);
    }

    void EditorLayer::ImGuiRender()
    {
        static bool opt_fullscreen_persistant = true;
        static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_None;
        bool opt_fullscreen = opt_fullscreen_persistant;

        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        auto boldFont = io.Fonts->Fonts[0];
        auto largeFont = io.Fonts->Fonts[1];

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

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", nullptr, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
        {
            ImGui::PopStyleVar(2);
        }

        // Dockspace
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 370.0f;
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), opt_flags);
        }

        style.WindowMinSize.x = minWinSizeX;

        // Editor Panel ------------------------------------------------------------------------------
        ImGui::Begin("Settings");
        {
            auto& rendererConfig = Renderer::GetConfig();

            UI::BeginPropertyGrid();
            ImGui::AlignTextToFramePadding();

            UI::PropertySlider("Skybox LOD", mEditorScene->GetSkyboxLod(), 0.0f, (float)Utils::CalculateMipCount(rendererConfig.EnvironmentMapResolution, rendererConfig.EnvironmentMapResolution));
            UI::PropertySlider("Exposure", mEditorCamera.GetExposure(), 0.0f, 5.0f);
            UI::PropertySlider("Env Map Rotation", mEnvMapRotation, -360.0f, 360.0f);
            UI::PropertySlider("Camera Speed", mEditorCamera.mNormalSpeed, 0.0005f, 2.f);

            if (mSceneState == SceneState::Edit)
            {
                float physics2DGravity = mEditorScene->GetPhysics2DGravity();
                float physics2DGravityDelta = physics2DGravity / 1000;
                if (UI::Property("Gravity", physics2DGravity, physics2DGravityDelta, -10000.0f, 10000.0f))
                {
                    mEditorScene->SetPhysics2DGravity(physics2DGravity);
                }
            }
            else if (mSceneState == SceneState::Play)
            {
                float physics2DGravity = mRuntimeScene->GetPhysics2DGravity();
                float physics2DGravityDelta = physics2DGravity / 1000;
                if (UI::Property("Gravity", physics2DGravity, physics2DGravityDelta, -10000.0f, 10000.0f))
                {
                    mRuntimeScene->SetPhysics2DGravity(physics2DGravity);
                }
            }

            if (UI::Property("Line Width", mLineWidth, 0.5f, 1.0f, 10.0f))
            {
                Renderer2D::SetLineWidth(mLineWidth);
            }

            UI::Property("Show Icons", mShowIcons);
            UI::Property("Show Bounding Boxes", mShowBoundingBoxes);
            {
                if (mShowBoundingBoxes)
                {
                    UI::Property("Selected Entity", mShowBoundingBoxSelectedMeshOnly);
                }
                if (mShowBoundingBoxes && mShowBoundingBoxSelectedMeshOnly)
                {
                    UI::Property("Submeshes", mShowBoundingBoxSubmeshes);
                }
                if (UI::Property("Show Selected Wireframe", mShowSelectedWireframe))
                {
                    UpdateSceneRendererSettings();
                }
                if (UI::Property("Show Physics Colliders", mShowPhysicsCollidersWireframe))
                {
                    UpdateSceneRendererSettings();
                }
                if (mShowPhysicsCollidersWireframe)
                {
                    if (UI::Property("-- On Top", mShowPhysicsCollidersWireframeOnTop))
                    {
                        UpdateSceneRendererSettings();
                    }
                }
            }
            UI::EndPropertyGrid();

            const char* label = mSelectionMode == SelectionMode::Entity ? "Entity" : "Mesh";
            if (ImGui::Button(label))
            {
                mSelectionMode = mSelectionMode == SelectionMode::Entity ? SelectionMode::SubMesh : SelectionMode::Entity;
            }

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
                    rendererConfig.EnvironmentMapResolution = (uint32_t)glm::pow(2, currentSize + 7);
                }
            }

            {
                const char* irradianceComputeSamples[] = { "128", "256", "512", "1024", "2048", "4096" };
                int currentSamples = (int)glm::log2((float)rendererConfig.IrradianceMapComputeSamples) - 7;
                if (UI::PropertyDropdown("Irradiance Map Compute Samples", irradianceComputeSamples, 6, &currentSamples))
                {
                    rendererConfig.IrradianceMapComputeSamples = (uint32_t)glm::pow(2, currentSamples + 7);
                }
            }
            UI::EndPropertyGrid();
        }
        ImGui::End();

        mContentBrowserPanel->ImGuiRender();
        mProjectSettingsPanel->ImGuiRender(mShowProjectSettings);
        mObjectsPanel->ImGuiRender();
        mConsolePanel->ImGuiRender(&mShowConsolePanel);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.305f, 0.31f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.1505f, 0.151f, 0.5f));

        ImGui::Begin("##tool_bar", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            float size = ImGui::GetWindowHeight() - 4.0F;
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
        }
        ImGui
            ::PopStyleColor(3);
        ImGui::PopStyleVar(2);
        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport");

        mViewportPanelMouseOver = ImGui::IsWindowHovered();
        mViewportPanelFocused = ImGui::IsWindowFocused();

        auto viewportOffset = ImGui::GetCursorPos(); // includes tab bar
        auto viewportSize = ImGui::GetContentRegionAvail();

        mViewportRenderer->SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
        mEditorScene->SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);

        if (mRuntimeScene)
        {
            mRuntimeScene->SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
        }

        mEditorCamera.SetProjectionMatrix(glm::perspectiveFov(glm::radians(45.0f), viewportSize.x, viewportSize.y, 0.1f, 1000.0f));
        mEditorCamera.SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);

        // Render viewport image
        UI::Image(mViewportRenderer->GetFinalPassImage(), viewportSize, { 0, 1 }, { 1, 0 });

        // Gizmo Toolbar
        {
            auto viewportStart = ImGui::GetItemRectMin();

            ImGui::SetNextWindowPos(ImVec2(viewportStart.x + 10, viewportStart.y + 5));
            ImGui::SetNextWindowSize(ImVec2(128, 28));

            ImGui::SetNextWindowBgAlpha(0.75f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 7.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 4));
            ImGui::Begin("##viewport_tools", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking);

            const ImVec4 cSelectedGizmoButtonColor = ImVec4(0.925490196f, 0.619607843f, 0.141176471f, 1.0f);
            const ImVec4 cUnselectedGizmoButtonColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

            if (UI::ImageButton(mSelectToolTex, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), mGizmoType == -1 ? cSelectedGizmoButtonColor : cUnselectedGizmoButtonColor))
            {
                mGizmoType = -1;
            }
            ImGui::SameLine();

            if (UI::ImageButton(mMoveToolTex, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), mGizmoType == ImGuizmo::OPERATION::TRANSLATE ? cSelectedGizmoButtonColor : cUnselectedGizmoButtonColor))
            {
                mGizmoType = ImGuizmo::OPERATION::TRANSLATE;
            }
            ImGui::SameLine();

            if (UI::ImageButton(mRotateToolTex, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), mGizmoType == ImGuizmo::OPERATION::ROTATE ? cSelectedGizmoButtonColor : cUnselectedGizmoButtonColor))
            {
                mGizmoType = ImGuizmo::OPERATION::ROTATE;
            }
            ImGui::SameLine();

            if (UI::ImageButton(mScaleToolTex, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), mGizmoType == ImGuizmo::OPERATION::SCALE ? cSelectedGizmoButtonColor : cUnselectedGizmoButtonColor))
            {
                mGizmoType = ImGuizmo::OPERATION::SCALE;
            }

            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);
            ImGui::End();
            ImGui::PopStyleVar(2);
        }

        static int counter = 0;
        auto windowSize = ImGui::GetWindowSize();
        ImVec2 minBound = ImGui::GetWindowPos();
        minBound.x += viewportOffset.x;
        minBound.y += viewportOffset.y;

        ImVec2 maxBound = { minBound.x + windowSize.x, minBound.y + windowSize.y };
        mViewportBounds[0] = { minBound.x, minBound.y };
        mViewportBounds[1] = { maxBound.x, maxBound.y };

        bool capturedLeftClick = minBound < io.MouseClickedPos[0] && io.MouseClickedPos[0] < maxBound;
        bool capturedRightClick = minBound < io.MouseClickedPos[1] && io.MouseClickedPos[1] < maxBound;
        bool capturedScrollWheelClick = minBound < io.MouseClickedPos[2] && io.MouseClickedPos[2] < maxBound;
        
        mViewportHasCapturedMouse = (capturedRightClick || capturedLeftClick || capturedScrollWheelClick) && mViewportPanelMouseOver;

        // Gizmos
        if (mGizmoType != -1 && mSelectionContext.size() && Input::GetCursorMode() == CursorMode::Normal)
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

                        glm::vec3 deltaRotation = (rotation * (180.0f / 3.14159265f)) - entityTransform.Rotation;
                        entityTransform.Translation = translation;
                        entityTransform.Rotation += deltaRotation;
                        entityTransform.Scale = scale;
                    }
                    else
                    {
                        glm::vec3 translation, rotation, scale;
                        Math::DecomposeTransform(transform, translation, rotation, scale);

                        glm::vec3 deltaRotation = (rotation * (180.0f / 3.14159265f)) - entityTransform.Rotation;
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

        if (ImGui::BeginDragDropTarget())
        {
            auto data = ImGui::AcceptDragDropPayload("asset_payload");
            if (data)
            {
                uint64_t count = data->DataSize / sizeof(AssetHandle);

                for (uint64_t i = 0; i < count; ++i)
                {
                    AssetHandle assetHandle = *((AssetHandle*)data->Data + i);
                    const AssetMetadata& assetData = AssetManager::GetMetadata(assetHandle);

                    // We can't really support dragging and dropping scenes when we're dropping multiple assets
                    if (count == 1 && assetData.Type == AssetType::Scene)
                    {
                        OpenScene(assetData);
                        break;
                    }

                    Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
                    if (asset)
                    {
                        if (asset->GetAssetType() == AssetType::MeshAsset)
                        {
                            mShowCreateNewMeshPopup = true;
                            mCreateNewMeshPopupData.MeshToCreate = asset.As<MeshAsset>();
                            mCreateNewMeshPopupData.TargetEntity = {};
                        }
                        else if (asset->GetAssetType() == AssetType::Mesh)
                        {
                            Entity entity = mEditorScene->CreateEntity(assetData.FilePath.stem().string());
                            entity.AddComponent<MeshComponent>(asset.As<Mesh>());
                            SelectEntity(entity);
                        }
                        else if (asset->GetAssetType() == AssetType::Prefab)
                        {
                            Ref<Prefab> prefab = asset.As<Prefab>();
                            mEditorScene->Instantiate(prefab);
                        }
                    }
                    else
                    {
                        mShowInvalidAssetMetadataPopup = true;
                        mInvalidAssetMetadataPopupData.Metadata = assetData;
                    }
                }
            }

            ImGui::EndDragDropTarget();
        }

        ImGui::End();
        ImGui::PopStyleVar();

        if (mShowSecondViewport)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::Begin("Second Viewport");

            mViewportPanel2MouseOver = ImGui::IsWindowHovered();
            mViewportPanel2Focused = ImGui::IsWindowFocused();

            auto viewportOffset = ImGui::GetCursorPos(); // includes tab bar
            auto viewportSize = ImGui::GetContentRegionAvail();
            if (viewportSize.x > 1 && viewportSize.y > 1)
            {
                mSecondViewportRenderer->SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
                mSecondEditorCamera.SetProjectionMatrix(glm::perspectiveFov(glm::radians(45.0f), viewportSize.x, viewportSize.y, 0.1f, 1000.0f));
                mSecondEditorCamera.SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);

                // Render viewport image
                UI::Image(mSecondViewportRenderer->GetFinalPassImage(), viewportSize, { 0, 1 }, { 1, 0 });

                static int counter = 0;
                auto windowSize = ImGui::GetWindowSize();
                ImVec2 minBound = ImGui::GetWindowPos();
                minBound.x += viewportOffset.x;
                minBound.y += viewportOffset.y;

                ImVec2 maxBound = { minBound.x + windowSize.x, minBound.y + windowSize.y };
                mSecondViewportBounds[0] = { minBound.x, minBound.y };
                mSecondViewportBounds[1] = { maxBound.x, maxBound.y };

                // Gizmos
                if (mGizmoType != -1 && mSelectionContext.size() && mViewportPanel2MouseOver)
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
                        ImGuizmo::Manipulate(glm::value_ptr(mSecondEditorCamera.GetViewMatrix()),
                            glm::value_ptr(mSecondEditorCamera.GetProjectionMatrix()),
                            (ImGuizmo::OPERATION)mGizmoType,
                            ImGuizmo::LOCAL,
                            glm::value_ptr(transform),
                            nullptr,
                            snap ? snapValues : nullptr);

                        if (ImGuizmo::IsUsing())
                        {
                            glm::vec3 translation, rotation, scale;
                            Math::DecomposeTransform(transform, translation, rotation, scale);

                            Entity parent = mCurrentScene->FindEntityByID(selection.EntityObj.GetParentID());
                            if (parent)
                            {
                                glm::vec3 parentTranslation, parentRotation, parentScale;
                                Math::DecomposeTransform(mCurrentScene->GetTransformRelativeToParent(parent), parentTranslation, parentRotation, parentScale);

                                glm::vec3 deltaRotation = (rotation - parentRotation) - entityTransform.Rotation;
                                entityTransform.Translation = translation - parentTranslation;
                                entityTransform.Rotation += deltaRotation;
                                entityTransform.Scale = scale;
                            }
                            else
                            {
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
                        ImGuizmo::Manipulate(glm::value_ptr(mSecondEditorCamera.GetViewMatrix()),
                            glm::value_ptr(mSecondEditorCamera.GetProjectionMatrix()),
                            (ImGuizmo::OPERATION)mGizmoType,
                            ImGuizmo::LOCAL,
                            glm::value_ptr(transformBase),
                            nullptr,
                            snap ? snapValues : nullptr);

                        selection.Mesh->Transform = glm::inverse(transform) * transformBase;
                    }
                }

                if (ImGui::BeginDragDropTarget())
                {
                    auto data = ImGui::AcceptDragDropPayload("asset_payload");
                    if (data)
                    {
                        int count = data->DataSize / sizeof(AssetHandle);

                        for (int i = 0; i < count; ++i)
                        {
                            AssetHandle assetHandle = *(((AssetHandle*)data->Data) + i);
                            const auto& assetData = AssetManager::GetMetadata(assetHandle);

                            if (count == 1 && assetData.Type == AssetType::Scene)
                            {
                                OpenScene(assetData.FilePath.string());
                                break;
                            }

                            Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
                            if (asset->GetAssetType() == AssetType::Mesh)
                            {
                                Entity entity = mEditorScene->CreateEntity(assetData.FilePath.stem().string());
                                entity.AddComponent<MeshComponent>(asset.As<Mesh>());
                                SelectEntity(entity);
                            }
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Create Project..."))
                {
                    mShowCreateNewProjectPopup = true;
                }
                if (ImGui::MenuItem("Save Project"))
                {
                    SaveProject();
                }
                if (ImGui::MenuItem("Open Project...", "Ctrl+O"))
                    OpenProject();
                if (ImGui::BeginMenu("Open Recent"))
                {
                    size_t i = 0;
                    for (auto it = mUserPreferences->RecentProjects.begin(); it != mUserPreferences->RecentProjects.end(); ++it)
                    {
                        if (i > 10)
                            break;
                        if (ImGui::MenuItem(it->second.Name.c_str()))
                        {
                            OpenProject(it->second.FilePath);
                            RecentProject projectEntry;
                            projectEntry.Name = it->second.Name;
                            projectEntry.FilePath = it->second.FilePath;
                            projectEntry.LastOpened = time(NULL);
                            it = mUserPreferences->RecentProjects.erase(it);
                            mUserPreferences->RecentProjects[projectEntry.LastOpened] = projectEntry;
                            UserPreferencesSerializer preferencesSerializer(mUserPreferences);
                            preferencesSerializer.Serialize(mUserPreferences->FilePath);
                            break;
                        }
                        ++i;
                    }
                    ImGui::EndMenu();
                }
                
                ImGui::Separator();

                if (ImGui::MenuItem("New Scene", "Ctrl+N"))
                {
                    NewScene();
                }
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

                if (ImGui::MenuItem("Exit", "Alt + F4"))
                {
                    Application::Get().Close();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                ImGui::MenuItem("Project Settings", nullptr, &mShowProjectSettings);
                ImGui::MenuItem("Second Viewport", nullptr, &mShowSecondViewport);
                if (ImGui::MenuItem("Reload C# Assembly"))
                {
                    ScriptEngine::ReloadAssembly((Project::GetScriptModuleFilePath()).string());
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("Audio Events Editor", nullptr, &mShowAudioEventsEditor);

                ImGui::MenuItem("Asset Manager", nullptr, &mAssetManagerPanelOpen);
                ImGui::EndMenu();
            }

#ifdef NR_DEBUG
            if (ImGui::BeginMenu("Debug"))
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
                        PhysicsDebugger::StartDebugging(
                            (Project::GetActive()->GetProjectDirectory() / "PhysicsDebugInfo").string(), 
                            PhysicsManager::GetSettings().DebugType == DebugType::LiveDebug
                        );
                    }
                }
                ImGui::EndMenu();
            }
#endif

            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("About"))
                {
                    mShowAboutPopup = true;
                }
                ImGui::EndMenu();
            }

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
                if (mesh && mesh->GetAssetType() == AssetType::Mesh)
                {
                    auto& materials = mesh->GetMaterials()->GetMaterials();
                    static uint32_t selectedMaterialIndex = 0;
                    for (uint32_t i = 0; i < materials.size(); ++i)
                    {
                        auto material = materials[i]->GetMaterial();
                        std::string materialName = material->GetName();
                        if (materialName.empty())
                        {
                            materialName = fmt::format("Unnamed Material #{0}", i);
                        }

                        ImGuiTreeNodeFlags node_flags = (selectedMaterialIndex == i ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_Leaf;
                        bool opened = ImGui::TreeNodeEx((void*)(&material), node_flags, materialName.c_str());
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

                    // Selected material
                    if (selectedMaterialIndex < materials.size())
                    {
                        auto material = materials[selectedMaterialIndex]->GetMaterial();
                        ImGui::Text("Shader: %s", material->GetShader()->GetName().c_str());
                        // Textures ------------------------------------------------------------------------------
                        {
                            // Albedo
                            if (ImGui::CollapsingHeader("Albedo", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                            {
                                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

                                auto& albedoColor = material->GetVector3("uMaterialUniforms.AlbedoColor");
                                bool useAlbedoMap = true;
                                Ref<Texture2D> albedoMap = material->TryGetTexture2D("uAlbedoTexture");
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
                                            if (asset->GetAssetType() != AssetType::Texture)
                                            {
                                                break;
                                            }

                                            albedoMap = asset.As<Texture2D>();
                                            material->Set("uAlbedoTexture", albedoMap);
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
                                            material->Set("uAlbedoTexture", albedoMap);
                                        }
                                    }
                                }
                                ImGui::SameLine();
                                ImGui::BeginGroup();
                                ImGui::Checkbox("Use##AlbedoMap", &useAlbedoMap);

                                ImGui::EndGroup();
                                ImGui::SameLine();
                                ImGui::ColorEdit3("Color##Albedo", glm::value_ptr(albedoColor), ImGuiColorEditFlags_NoInputs);
                                float& emission = material->GetFloat("uMaterialUniforms.Emission");
                                ImGui::DragFloat("Emission", &emission);
                            }
                        }
                        {
                            // Normals
                            if (ImGui::CollapsingHeader("Normals", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                            {
                                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                                bool useNormalMap = material->GetFloat("uMaterialUniforms.UseNormalMap");
                                Ref<Texture2D> normalMap = material->TryGetTexture2D("uNormalTexture");
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
                                            if (asset->GetAssetType() != AssetType::Texture)
                                            {
                                                break;
                                            }

                                            normalMap = asset.As<Texture2D>();
                                            material->Set("uNormalTexture", normalMap);
                                            material->Set("uMaterialUniforms.UseNormalMap", true);
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
                                            material->Set("uNormalTexture", normalMap);
                                        }
                                    }
                                }
                                ImGui::SameLine();
                                if (ImGui::Checkbox("Use##NormalMap", &useNormalMap))
                                {
                                    material->Set("uMaterialUniforms.UseNormalMap", useNormalMap);
                                }
                            }
                        }
                        {
                            // Metalness
                            if (ImGui::CollapsingHeader("Metalness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                            {
                                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                                float& metalnessValue = material->GetFloat("uMaterialUniforms.Metalness");
                                bool useMetalnessMap = true;
                                Ref<Texture2D> metalnessMap = material->TryGetTexture2D("uMetalnessTexture");
                                UI::Image((metalnessMap && metalnessMap->GetImage()) ? metalnessMap : mCheckerboardTex, ImVec2(64, 64));

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
                                            if (asset->GetAssetType() != AssetType::Texture)
                                            {
                                                break;
                                            }

                                            metalnessMap = asset.As<Texture2D>();
                                            material->Set("uMetalnessTexture", metalnessMap);
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
                                            material->Set("uMetalnessTexture", metalnessMap);
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
                                float& roughnessValue = material->GetFloat("uMaterialUniforms.Roughness");
                                bool useRoughnessMap = true;
                                Ref<Texture2D> roughnessMap = material->TryGetTexture2D("uRoughnessTexture");
                                UI::Image((roughnessMap && roughnessMap->GetImage()) ? roughnessMap : mCheckerboardTex, ImVec2(64, 64));

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
                                            if (asset->GetAssetType() != AssetType::Texture)
                                            {
                                                break;
                                            }

                                            roughnessMap = asset.As<Texture2D>();
                                            material->Set("uRoughnessTexture", roughnessMap);
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
                                            material->Set("uRoughnessTexture", roughnessMap);
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
        if (mViewportPanelFocused)
        {
            mFocusedRenderer = mViewportRenderer;
        }
        else if (mViewportPanel2Focused)
        {
            mFocusedRenderer = mSecondViewportRenderer;
        }

        mFocusedRenderer->ImGuiRender();
        SceneAudio::ImGuiRender();
        AudioEventsEditor::ImGuiRender(mShowAudioEventsEditor);

        ImGui::End();

        {
            if (sNotRedInstallPath.empty() && !ImGui::IsPopupOpen("Select NotRed Install"))
            {
                ImGui::OpenPopup("Select NotRed Install");
                sNotRedInstallPath.reserve(MAX_PROJECT_FILEPATH_LENGTH);
            }

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(700, 0));
            if (ImGui::BeginPopupModal("Select NotRed Install", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
            {
                ImGui::PushFont(boldFont);
                ImGui::TextUnformatted("Failed to find an appropiate NotRed installation!");
                ImGui::PopFont();

                ImGui::TextWrapped("Please select the root folder for the NotRed version you want to use (E.g C:/Dev/NotRed-dev).You should be able to find a file called premake5.lua in the root folder.");
                ImGui::Dummy(ImVec2(0, 8));
                ImVec2 label_size = ImGui::CalcTextSize("...", NULL, true);
                auto& style = ImGui::GetStyle();
                ImVec2 button_size = ImGui::CalcItemSize(ImVec2(0, 0), label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 10));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 6));
                ImGui::SetNextItemWidth(700 - button_size.x - style.FramePadding.x * 2.0f - style.ItemInnerSpacing.x - 1);
                ImGui::InputTextWithHint("##notred_install_location", "C:/Dev/NotRed-dev/", sNotRedInstallPath.data(), MAX_PROJECT_FILEPATH_LENGTH, ImGuiInputTextFlags_ReadOnly);
                ImGui::SameLine();
                if (ImGui::Button("..."))
                {
                    std::string result = Application::Get().OpenFolder();
                    sNotRedInstallPath.assign(result);
                }
                if (ImGui::Button("Confirm"))
                {
                    bool success = FileSystem::SetEnvironmentVariable("NOTRED_DIR", sNotRedInstallPath);
                    NR_CORE_ASSERT(success, "Failed to set Environment Variable!");
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopStyleVar(2);
                ImGui::EndPopup();
            }
        }
        {
            if (mShowCreateNewProjectPopup)
            {
                ImGui::OpenPopup("New Project");
                memset(sProjectNameBuffer, 0, MAX_PROJECT_NAME_LENGTH);
                memset(sProjectFilePathBuffer, 0, MAX_PROJECT_FILEPATH_LENGTH);
                mShowCreateNewProjectPopup = false;
            }

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2{ 700, 325 });
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 10));

            if (ImGui::BeginPopupModal("New Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
            {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 325 / 8);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 7));
                ImGui::PushFont(boldFont);
                std::string fullProjectPath = strlen(sProjectFilePathBuffer) > 0 ? std::string(sProjectFilePathBuffer) + "/" + std::string(sProjectNameBuffer) : "";
                ImGui::Text("Full Project Path: %s", fullProjectPath.c_str());
                ImGui::PopFont();
                ImGui::SetNextItemWidth(-1);
                ImGui::InputTextWithHint("##new_project_name", "Project Name", sProjectNameBuffer, MAX_PROJECT_NAME_LENGTH);
                ImVec2 label_size = ImGui::CalcTextSize("...", NULL, true);
                auto& style = ImGui::GetStyle();
                ImVec2 button_size = ImGui::CalcItemSize(ImVec2(0, 0), label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);
                ImGui::SetNextItemWidth(600 - button_size.x - style.FramePadding.x * 2.0f - style.ItemInnerSpacing.x - 1);
                ImGui::InputTextWithHint("##new_project_location", "Project Location", sProjectFilePathBuffer, MAX_PROJECT_FILEPATH_LENGTH, ImGuiInputTextFlags_ReadOnly);
                
                ImGui::SameLine();
                
                if (ImGui::Button("..."))
                {
                    std::string result = Application::Get().OpenFolder();
                    std::replace(result.begin(), result.end(), '\\', '/');
                    memcpy(sProjectFilePathBuffer, result.data(), result.length());
                }

                ImGui::Separator();
                ImGui::PushFont(boldFont);
                if (ImGui::Button("Create"))
                {
                    CreateProject(fullProjectPath);
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    ImGui::CloseCurrentPopup();
                }

                ImGui::PopFont();
                ImGui::PopStyleVar();
                ImGui::EndPopup();
            }
            ImGui::PopStyleVar(2);
        }

        UI_WelcomePopup();
        UI_AboutPopup();

        UI_CreateNewMeshPopup();
        UI_InvalidAssetMetadataPopup();

        AssetEditorPanel::ImGuiRender();
        AssetManager::ImGuiRender(mAssetManagerPanelOpen);
    }

    void EditorLayer::OnEvent(Event& e)
    {
        if (mSceneState == SceneState::Edit)
        {
            if (mViewportPanelMouseOver)
            {
                mEditorCamera.OnEvent(e);
            }

            if (mViewportPanel2MouseOver)
            {
                mSecondEditorCamera.OnEvent(e);
            }

            mEditorScene->OnEvent(e);
        }
        else if (mSceneState == SceneState::Play)
        {
            mRuntimeScene->OnEvent(e);
        }

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& event) { return OnKeyPressedEvent(event); });
        dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& event) { return OnMouseButtonPressed(event); });

        AssetEditorPanel::OnEvent(e);
        mContentBrowserPanel->OnEvent(e);
    }

    bool EditorLayer::OnKeyPressedEvent(KeyPressedEvent& e)
    {
        auto isAudioEventsEditorFocused = []
            {
                // Note: child name usually starts with "parent window name"/"child name"
                auto* audioEventsEditorWindow = ImGui::FindWindowByName("Audio Events Editor");
                if (GImGui->NavWindow)
                {
                    return GImGui->NavWindow == audioEventsEditorWindow || Utils::StartsWith(GImGui->NavWindow->Name, "Audio Events Editor");
                }
                else
                {
                    return false;
                }
            };

        if (isAudioEventsEditorFocused())
        {
            if (AudioEventsEditor::OnKeyPressedEvent(e))
            {
                return true;
            }
        }

        if (GImGui->ActiveId == 0)
        {
            if ((mViewportPanelMouseOver || mViewportPanel2MouseOver) && !Input::IsMouseButtonPressed(MouseButton::Right))
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
            case KeyCode::Delete: // TODO: this should be in the scene hierarchy panel
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

        if (Input::IsKeyPressed(NR_KEY_LEFT_CONTROL) && !Input::IsMouseButtonPressed(MouseButton::Right))
        {
            switch (e.GetKeyCode())
            {
            case KeyCode::B:
            {
                mShowBoundingBoxes = !mShowBoundingBoxes;
            }
            break;
            case KeyCode::D:
                if (mSelectionContext.size())
                {
                    Entity selectedEntity = mSelectionContext[0].EntityObj;
                    mEditorScene->DuplicateEntity(selectedEntity);
                }
                break;
            case KeyCode::G:
            {
                mViewportRenderer->GetOptions().ShowGrid = !mViewportRenderer->GetOptions().ShowGrid;
                break;
            }
            case KeyCode::P:
            {
                if (mViewportRenderer->GetOptions().ShowPhysicsColliders != SceneRendererOptions::PhysicsColliderView::None)
                {
                    mViewportRenderer->GetOptions().ShowPhysicsColliders = SceneRendererOptions::PhysicsColliderView::None;
                }
                else
                {
                    mViewportRenderer->GetOptions().ShowPhysicsColliders = SceneRendererOptions::PhysicsColliderView::Normal;
                }
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
                SaveScene();
                break;
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
        if (e.GetMouseButton() == NR_MOUSE_BUTTON_LEFT && (mViewportPanelMouseOver || mViewportPanel2MouseOver) && !Input::IsKeyPressed(KeyCode::LeftAlt) && !Input::IsMouseButtonPressed(MouseButton::Right) && !ImGuizmo::IsOver() && mSceneState != SceneState::Play)
        {
            ImGui::ClearActiveID();
            auto [mouseX, mouseY] = GetMouseViewportSpace(mViewportPanelMouseOver);
            if (mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f)
            {
                const auto& camera = mViewportPanelMouseOver ? mEditorCamera : mSecondEditorCamera;
                auto [origin, direction] = CastRay(camera, mouseX, mouseY);

                mSelectionContext.clear();
                mCurrentScene->SetSelectedEntity({});
                auto meshEntities = mCurrentScene->GetAllEntitiesWith<MeshComponent>();
                for (auto e : meshEntities)
                {
                    Entity entity = { e, mCurrentScene.Raw() };
                    auto mesh = entity.GetComponent<MeshComponent>().MeshObj;
                    if (!mesh || mesh->IsFlagSet(AssetFlag::Missing))
                    {
                        continue;
                    }

                    auto& submeshes = mesh->GetMeshAsset()->GetSubmeshes();
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
                            const auto& triangleCache = mesh->GetMeshAsset()->GetTriangleCache(i);
                            for (const auto& triangle : triangleCache)
                            {
                                if (ray.IntersectsTriangle(triangle.V0.Position, triangle.V1.Position, triangle.V2.Position, t))
                                {
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

    std::pair<float, float> EditorLayer::GetMouseViewportSpace(bool primaryViewport)
    {
        auto [mx, my] = ImGui::GetMousePos();
        const auto& viewportBounds = primaryViewport ? mViewportBounds : mSecondViewportBounds;

        mx -= viewportBounds[0].x;
        my -= viewportBounds[0].y;

        auto viewportWidth = viewportBounds[1].x - viewportBounds[0].x;
        auto viewportHeight = viewportBounds[1].y - viewportBounds[0].y;

        return { (mx / viewportWidth) * 2.0f - 1.0f, ((my / viewportHeight) * 2.0f - 1.0f) * -1.0f };
    }

    std::pair<glm::vec3, glm::vec3> EditorLayer::CastRay(const EditorCamera& camera, float mx, float my)
    {
        glm::vec4 mouseClipPos = { mx, my, -1.0f, 1.0f };

        auto inverseProj = glm::inverse(camera.GetProjectionMatrix());
        auto inverseView = glm::inverse(glm::mat3(camera.GetViewMatrix()));

        glm::vec4 ray = inverseProj * mouseClipPos;
        glm::vec3 rayPos = camera.GetPosition();
        glm::vec3 rayDir = inverseView * glm::vec3(ray);

        return { rayPos, rayDir };
    }

    void EditorLayer::Selected(const SelectedSubmesh& selectionContext)
    {
        mSceneHierarchyPanel->SetSelected(selectionContext.EntityObj);
        mEditorScene->SetSelectedEntity(selectionContext.EntityObj);
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
#if 0
        auto [mouseX, mouseY] = GetMouseViewportSpace();
        if (mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f)
        {
            auto [origin, direction] = CastRay(mouseX, mouseY);
            return Ray(origin, direction);
        }
#endif
        NR_CORE_ASSERT(false);
        return Ray::Zero();
    }
}