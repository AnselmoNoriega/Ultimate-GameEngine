#include "EditorLayer.h"

#include <filesystem>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/imgui_internal.h>

#include "NotRed/ImGui/ImGuizmo.h"
#include "NotRed/Renderer/Renderer2D.h"
#include "NotRed/Script/ScriptEngine.h"

#include "NotRed/Physics/PhysicsManager.h"
#include "NotRed/Editor/PhysicsSettingsWindow.h"
#include "NotRed/Math/Math.h"

namespace NR
{
    static void ImGuiShowHelpMarker(const char* desc)
    {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

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

        mSceneHierarchyPanel = CreateScope<SceneHierarchyPanel>(mEditorScene);
        mSceneHierarchyPanel->SetSelectionChangedCallback(std::bind(&EditorLayer::SelectEntity, this, std::placeholders::_1));
        mSceneHierarchyPanel->SetEntityDeletedCallback(std::bind(&EditorLayer::EntityDeleted, this, std::placeholders::_1));

        NewScene();
    }

    void EditorLayer::Detach()
    {

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
    }

    void EditorLayer::SceneStop()
    {
        mRuntimeScene->RuntimeStop();
        mSceneState = SceneState::Edit;

        mRuntimeScene = nullptr;

        mSelectionContext.clear();
        ScriptEngine::SetSceneContext(mEditorScene);
        mSceneHierarchyPanel->SetContext(mEditorScene);

        Input::SetCursorMode(CursorMode::Normal);
    }

    void EditorLayer::UpdateWindowTitle(const std::string& sceneName)
    {
        std::string title = sceneName + " - NotEditor - " + Application::GetPlatformName() + " (" + Application::GetConfigurationName() + ")";
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

    void EditorLayer::Update(float dt)
    {
        auto [x, y] = GetMouseViewportSpace();

        SceneRenderer::SetFocusPoint({ x * 0.5f + 0.5f, y * 0.5f + 0.5f });

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

                if (selection.EntityObj.HasComponent<BoxCollider2DComponent>())
                {
                    const auto& size = selection.EntityObj.GetComponent<BoxCollider2DComponent>().Size;
                    const TransformComponent& transform = selection.EntityObj.GetComponent<TransformComponent>();

                    Renderer::BeginRenderPass(SceneRenderer::GetFinalRenderPass(), false);
                    auto viewProj = mEditorCamera.GetViewProjection();
                    Renderer2D::BeginScene(viewProj, false);
                    Renderer2D::DrawRotatedQuad({ transform.Translation.x, transform.Translation.y }, size * 2.0f, transform.Rotation.z, { 1.0f, 0.0f, 1.0f, 1.0f });
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
        }
    }

    bool EditorLayer::Property(const std::string& name, bool& value)
    {
        ImGui::Text(name.c_str());
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        std::string id = "##" + name;
        ImGui::Checkbox(id.c_str(), &value);
        bool result = ImGui::Checkbox(id.c_str(), &value);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return result;
    }

    bool EditorLayer::Property(const std::string& name, float& value, float min, float max, EditorLayer::PropertyFlag flags)
    {
        ImGui::Text(name.c_str());
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        std::string id = "##" + name;
        bool changed = false;
        if (flags == PropertyFlag::SliderProperty)
        {
            changed = ImGui::SliderFloat(id.c_str(), &value, min, max);
        }
        else
        {
            changed = ImGui::DragFloat(id.c_str(), &value, 1.0f, min, max);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return changed;
    }

    bool EditorLayer::Property(const std::string& name, glm::vec2& value, EditorLayer::PropertyFlag flags)
    {
        return Property(name, value, -1.0f, 1.0f, flags);
    }

    bool EditorLayer::Property(const std::string& name, glm::vec2& value, float min, float max, EditorLayer::PropertyFlag flags)
    {
        ImGui::Text(name.c_str());
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        std::string id = "##" + name;
        bool changed = false;
        if (flags == PropertyFlag::SliderProperty)
        {
            changed = ImGui::SliderFloat2(id.c_str(), glm::value_ptr(value), min, max);
        }
        else
        {
            changed = ImGui::DragFloat2(id.c_str(), glm::value_ptr(value), 1.0f, min, max);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return changed;
    }

    bool EditorLayer::Property(const std::string& name, glm::vec3& value, EditorLayer::PropertyFlag flags)
    {
        return Property(name, value, -1.0f, 1.0f, flags);
    }

    bool EditorLayer::Property(const std::string& name, glm::vec3& value, float min, float max, EditorLayer::PropertyFlag flags)
    {
        ImGui::Text(name.c_str());
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        std::string id = "##" + name;
        bool changed = false;
        if ((int)flags & (int)PropertyFlag::ColorProperty)
        {
            changed = ImGui::ColorEdit3(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
        }
        else if (flags == PropertyFlag::SliderProperty)
        {
            changed = ImGui::SliderFloat3(id.c_str(), glm::value_ptr(value), min, max);
        }
        else
        {
            changed = ImGui::DragFloat3(id.c_str(), glm::value_ptr(value), 1.0f, min, max);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return changed;
    }

    bool EditorLayer::Property(const std::string& name, glm::vec4& value, EditorLayer::PropertyFlag flags)
    {
        return Property(name, value, -1.0f, 1.0f, flags);
    }

    bool EditorLayer::Property(const std::string& name, glm::vec4& value, float min, float max, EditorLayer::PropertyFlag flags)
    {
        ImGui::Text(name.c_str());
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        std::string id = "##" + name;
        bool changed = false;
        if ((int)flags & (int)PropertyFlag::ColorProperty)
        {
            changed = ImGui::ColorEdit4(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
        }
        else if (flags == PropertyFlag::SliderProperty)
        {
            changed = ImGui::SliderFloat4(id.c_str(), glm::value_ptr(value), min, max);
        }
        else
        {
            changed = ImGui::DragFloat4(id.c_str(), glm::value_ptr(value), 1.0f, min, max);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return changed;
    }

    void EditorLayer::NewScene()
    {
        mEditorScene = Ref<Scene>::Create();
        mSceneHierarchyPanel->SetContext(mEditorScene);
        ScriptEngine::SetSceneContext(mEditorScene);
        UpdateWindowTitle("Scene");
        mSceneFilePath = std::string();

        mEditorCamera = EditorCamera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f));
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

        mEditorScene->SetSelectedEntity({});
        mSelectionContext.clear();
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
        std::string filepath = app.SaveFile("NotRed Scene (*.nsc)\0*.nsc\0");
        if (!filepath.empty())
        {
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

        static bool opt_fullscreen_persistant = true;
        static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_None;
        bool opt_fullscreen = opt_fullscreen_persistant;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
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

        // When using ImGuiDockNodeFlags_PassthruDockspace, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
        ImGuiDockNodeFlags ImGuiDockNodeFlags_PassthruDockspace = ImGuiDockNodeFlags_NoSplit | ImGuiDockNodeFlags_NoResize | ImGuiDockNodeFlags_NoDockingInCentralNode;
        //if (opt_flags & ImGuiDockNodeFlags_PassthruDockspace)
        //{
        //	window_flags |= ImGuiWindowFlags_NoBackground;
        //}

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &p_open, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
        {
            ImGui::PopStyleVar(2);
        }

        // Dockspace
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 370.0f;
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), opt_flags);
        }

        style.WindowMinSize.x = minWinSizeX;

        // Editor Panel ------------------------------------------------------------------------------
        ImGui::Begin("Model");

        ImGui::Begin("Environment");

        ImGui::SliderFloat("Skybox LOD", &mEditorScene->GetSkyboxLod(), 0.0f, 11.0f);

        ImGui::Columns(2);
        ImGui::AlignTextToFramePadding();

        auto& light = mEditorScene->GetLight();
        Property("Light Direction", light.Direction, PropertyFlag::SliderProperty);
        Property("Light Radiance", light.Radiance, PropertyFlag::ColorProperty);
        Property("Light Multiplier", light.Multiplier, 0.0f, 5.0f, PropertyFlag::SliderProperty);
        Property("Exposure", mEditorCamera.GetExposure(), 0.0f, 5.0f, PropertyFlag::SliderProperty);

        Property("Radiance Prefiltering", mRadiancePrefilter);
        Property("Env Map Rotation", mEnvMapRotation, -360.0f, 360.0f, PropertyFlag::SliderProperty);

        if (mSceneState == SceneState::Edit)
        {
            float physics2DGravity = mEditorScene->GetPhysics2DGravity();
            if (Property("Gravity", physics2DGravity, -10000.0f, 10000.0f, PropertyFlag::DragProperty))
            {
                mEditorScene->SetPhysics2DGravity(physics2DGravity);
            }
        }
        else if (mSceneState == SceneState::Play)
        {
            float physics2DGravity = mRuntimeScene->GetPhysics2DGravity();
            if (Property("Gravity", physics2DGravity, -10000.0f, 10000.0f, PropertyFlag::DragProperty))
            {
                mRuntimeScene->SetPhysics2DGravity(physics2DGravity);
            }
        }

        if (Property("Show Bounding Boxes", mUIShowBoundingBoxes))
        {
            ShowBoundingBoxes(mUIShowBoundingBoxes, mUIShowBoundingBoxesOnTop);
        }
        if (mUIShowBoundingBoxes && Property("On Top", mUIShowBoundingBoxesOnTop))
        {
            ShowBoundingBoxes(mUIShowBoundingBoxes, mUIShowBoundingBoxesOnTop);
        }

        const char* label = (mSelectionMode == SelectionMode::Entity) ? "Entity" : "Mesh";
        if (ImGui::Button(label))
        {
            mSelectionMode = (mSelectionMode == SelectionMode::Entity) ? SelectionMode::SubMesh : SelectionMode::Entity;
        }

        ImGui::Columns(1);

        ImGui::End();

        ImGui::Separator();
        {
            ImGui::Text("Mesh");

        }
        ImGui::Separator();

        if (ImGui::TreeNode("Shaders"))
        {
            auto& shaders = Shader::sAllShaders;
            for (auto& shader : shaders)
            {
                if (ImGui::TreeNode(shader->GetName().c_str()))
                {
                    std::string buttonName = "Reload##" + shader->GetName();
                    if (ImGui::Button(buttonName.c_str()))
                        shader->Reload();
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }

        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0.8f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
        ImGui::Begin("Toolbar");
        if (mSceneState == SceneState::Edit)
        {
            if (ImGui::ImageButton((ImTextureID)(mPlayButtonTex->GetRendererID()), ImVec2(32, 32), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), ImVec4(0.9f, 0.9f, 0.9f, 1.0f)))
            {
                ScenePlay();
            }
        }
        else if (mSceneState == SceneState::Play)
        {
            if (ImGui::ImageButton((ImTextureID)(mPlayButtonTex->GetRendererID()), ImVec2(32, 32), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(1.0f, 1.0f, 1.0f, 0.2f)))
            {
                SceneStop();
            }
        }
        ImGui::SameLine();
        if (ImGui::ImageButton((ImTextureID)(mPlayButtonTex->GetRendererID()), ImVec2(32, 32), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), ImVec4(1.0f, 1.0f, 1.0f, 0.6f)))
        {
            NR_CORE_INFO("PLAY!");
        }
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();

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
        ImGui::Image((void*)SceneRenderer::GetFinalColorBufferRendererID(), viewportSize, { 0, 1 }, { 1, 0 });

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
            glm::mat4 transform = entityTransform.GetTransform();
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
                    glm::vec3 translation, rotation, scale;
                    Math::DecomposeTransform(transform, translation, rotation, scale);

                    glm::vec3 deltaRotation = rotation - entityTransform.Rotation;
                    entityTransform.Translation = translation;
                    entityTransform.Rotation += deltaRotation;
                    entityTransform.Scale = scale;
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
                if (mesh)
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

                                auto& albedoColor = materialInstance->Get<glm::vec3>("uAlbedoColor");
                                bool useAlbedoMap = materialInstance->Get<float>("uAlbedoTexToggle");
                                Ref<Texture2D> albedoMap = materialInstance->TryGetResource<Texture2D>("uAlbedoTexture");
                                ImGui::Image(albedoMap ? (void*)albedoMap->GetRendererID() : (void*)mCheckerboardTex->GetRendererID(), ImVec2(64, 64));
                                ImGui::PopStyleVar();
                                if (ImGui::IsItemHovered())
                                {
                                    if (albedoMap)
                                    {
                                        ImGui::BeginTooltip();
                                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                        ImGui::TextUnformatted(albedoMap->GetPath().c_str());
                                        ImGui::PopTextWrapPos();
                                        ImGui::Image((void*)albedoMap->GetRendererID(), ImVec2(384, 384));
                                        ImGui::EndTooltip();
                                    }
                                    if (ImGui::IsItemClicked())
                                    {
                                        std::string filename = Application::Get().OpenFile("");
                                        if (filename != "")
                                        {
                                            albedoMap = Texture2D::Create(filename, true/*m_AlbedoInput.SRGB*/);
                                            materialInstance->Set("uAlbedoTexture", albedoMap);
                                        }
                                    }
                                }
                                ImGui::SameLine();
                                ImGui::BeginGroup();
                                if (ImGui::Checkbox("Use##AlbedoMap", &useAlbedoMap))
                                {
                                    materialInstance->Set<float>("uAlbedoTexToggle", useAlbedoMap ? 1.0f : 0.0f);
                                }

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
                                bool useNormalMap = materialInstance->Get<float>("uNormalTexToggle");
                                Ref<Texture2D> normalMap = materialInstance->TryGetResource<Texture2D>("uNormalTexture");
                                ImGui::Image(normalMap ? (void*)normalMap->GetRendererID() : (void*)mCheckerboardTex->GetRendererID(), ImVec2(64, 64));
                                ImGui::PopStyleVar();
                                if (ImGui::IsItemHovered())
                                {
                                    if (normalMap)
                                    {
                                        ImGui::BeginTooltip();
                                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                        ImGui::TextUnformatted(normalMap->GetPath().c_str());
                                        ImGui::PopTextWrapPos();
                                        ImGui::Image((void*)normalMap->GetRendererID(), ImVec2(384, 384));
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
                                    materialInstance->Set<float>("uNormalTexToggle", useNormalMap ? 1.0f : 0.0f);
                                }
                            }
                        }
                        {
                            // Metalness
                            if (ImGui::CollapsingHeader("Metalness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                            {
                                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                                float& metalnessValue = materialInstance->Get<float>("uMetalness");
                                bool useMetalnessMap = materialInstance->Get<float>("uMetalnessTexToggle");
                                Ref<Texture2D> metalnessMap = materialInstance->TryGetResource<Texture2D>("uMetalnessTexture");
                                ImGui::Image(metalnessMap ? (void*)metalnessMap->GetRendererID() : (void*)mCheckerboardTex->GetRendererID(), ImVec2(64, 64));
                                ImGui::PopStyleVar();
                                if (ImGui::IsItemHovered())
                                {
                                    if (metalnessMap)
                                    {
                                        ImGui::BeginTooltip();
                                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                        ImGui::TextUnformatted(metalnessMap->GetPath().c_str());
                                        ImGui::PopTextWrapPos();
                                        ImGui::Image((void*)metalnessMap->GetRendererID(), ImVec2(384, 384));
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
                                if (ImGui::Checkbox("Use##MetalnessMap", &useMetalnessMap))
                                    materialInstance->Set<float>("uMetalnessTexToggle", useMetalnessMap ? 1.0f : 0.0f);
                                ImGui::SameLine();
                                ImGui::SliderFloat("Value##MetalnessInput", &metalnessValue, 0.0f, 1.0f);
                            }
                        }
                        {
                            // Roughness
                            if (ImGui::CollapsingHeader("Roughness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                            {
                                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                                float& roughnessValue = materialInstance->Get<float>("uRoughness");
                                bool useRoughnessMap = materialInstance->Get<float>("uRoughnessTexToggle");
                                Ref<Texture2D> roughnessMap = materialInstance->TryGetResource<Texture2D>("uRoughnessTexture");
                                ImGui::Image(roughnessMap ? (void*)roughnessMap->GetRendererID() : (void*)mCheckerboardTex->GetRendererID(), ImVec2(64, 64));
                                ImGui::PopStyleVar();
                                if (ImGui::IsItemHovered())
                                {
                                    if (roughnessMap)
                                    {
                                        ImGui::BeginTooltip();
                                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                        ImGui::TextUnformatted(roughnessMap->GetPath().c_str());
                                        ImGui::PopTextWrapPos();
                                        ImGui::Image((void*)roughnessMap->GetRendererID(), ImVec2(384, 384));
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
                                if (ImGui::Checkbox("Use##RoughnessMap", &useRoughnessMap))
                                    materialInstance->Set<float>("uRoughnessTexToggle", useRoughnessMap ? 1.0f : 0.0f);
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
        PhysicsSettingsWindow::ImGuiRender(mShowPhysicsSettings);

        ImGui::End();
    }

    void EditorLayer::ShowBoundingBoxes(bool show, bool onTop)
    {
        SceneRenderer::GetOptions().ShowBoundingBoxes = show && !onTop;
        mDrawOnTopBoundingBoxes = show && onTop;
    }


    void EditorLayer::SelectEntity(Entity entity)
    {
        SelectedSubmesh selection;
        if (entity.HasComponent<MeshComponent>())
        {
            auto& meshComp = entity.GetComponent<MeshComponent>();

            if (meshComp.MeshObj)
            {
                selection.Mesh = &meshComp.MeshObj->GetSubmeshes()[0];
            }
        }
        selection.EntityObj = entity;
        mSelectionContext.clear();
        mSelectionContext.push_back(selection);

        mEditorScene->SetSelectedEntity(entity);
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
                }

            }
            switch (e.GetKeyCode())
            {
            case KeyCode::Delete:
                if (mSelectionContext.size())
                {
                    Entity selectedEntity = mSelectionContext[0].EntityObj;
                    mEditorScene->DestroyEntity(selectedEntity);
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
                mEditorScene->SetSelectedEntity({});
                auto meshEntities = mEditorScene->GetAllEntitiesWith<MeshComponent>();
                for (auto e : meshEntities)
                {
                    Entity entity = { e, mEditorScene.Raw() };
                    auto mesh = entity.GetComponent<MeshComponent>().MeshObj;
                    if (!mesh)
                    {
                        continue;
                    }

                    auto& submeshes = mesh->GetSubmeshes();
                    constexpr float lastT = std::numeric_limits<float>::max();
                    for (uint32_t i = 0; i < submeshes.size(); ++i)
                    {
                        auto& submesh = submeshes[i];
                        Ray ray = {
                            glm::inverse(entity.Transform().GetTransform() * submesh.Transform) * glm::vec4(origin, 1.0f),
                            glm::inverse(glm::mat3(entity.Transform().GetTransform()) * glm::mat3(submesh.Transform)) * direction
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
        mEditorScene->SetSelectedEntity(selectionContext.EntityObj);
    }

    void EditorLayer::EntityDeleted(Entity e)
    {
        if (mSelectionContext[0].EntityObj == e)
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