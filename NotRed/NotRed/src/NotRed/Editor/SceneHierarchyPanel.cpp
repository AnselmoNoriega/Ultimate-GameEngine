#include "nrpch.h"
#include "SceneHierarchyPanel.h"

#include <imgui.h>
#include <imgui/imgui_internal.h>

#include <assimp/scene.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "NotRed/ImGui/ImGui.h"

#include "NotRed/Core/Application.h"
#include "NotRed/Script/ScriptEngine.h"
#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Renderer/MeshFactory.h"

#include "NotRed/Physics/PhysicsWrappers.h"
#include "NotRed/Physics/PhysicsManager.h"
#include "NotRed/Physics/PhysicsLayer.h"
#include "NotRed/Physics/PhysicsActor.h"

namespace NR
{
    glm::mat4 AssimpMat4ToMat4(const aiMatrix4x4& matrix);

    SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
        : mContext(context)
    {
    }

    void SceneHierarchyPanel::SetContext(const Ref<Scene>& scene)
    {
        mContext = scene;
        mSelectionContext = {};
        if (mSelectionContext && false)
        {
            auto& entityMap = mContext->GetEntityMap();
            UUID selectedEntityID = mSelectionContext.GetID();
            if (entityMap.find(selectedEntityID) != entityMap.end())
            {
                mSelectionContext = entityMap.at(selectedEntityID);
            }
        }
    }

    void SceneHierarchyPanel::SetSelected(Entity entity)
    {
        mSelectionContext = entity;
    }

    void SceneHierarchyPanel::ImGuiRender()
    {
        ImGui::Begin("Scene Hierarchy");

        if (mContext)
        {
            uint32_t entityCount = 0, meshCount = 0;
            auto view = mContext->mRegistry.view<IDComponent>();
            view.each([&](entt::entity entity, auto id)
                {
                    DrawEntityNode(Entity(entity, mContext.Raw()));
                });

            if (ImGui::BeginPopupContextWindow(0, 1))
            {
                if (ImGui::BeginMenu("Create"))
                {
                    if (ImGui::MenuItem("Empty Entity"))
                    {
                        auto newEntity = mContext->CreateEntity("Entity");
                        SetSelected(newEntity);
                    }
                    if (ImGui::MenuItem("Mesh"))
                    {
                        auto newEntity = mContext->CreateEntity("Entity");
                        newEntity.AddComponent<MeshComponent>();
                        SetSelected(newEntity);
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Directional Light"))
                    {
                        auto newEntity = mContext->CreateEntity("Directional Light");
                        newEntity.AddComponent<DirectionalLightComponent>();
                        newEntity.GetComponent<TransformComponent>().GetTransform() = glm::toMat4(glm::quat(glm::radians(glm::vec3{ 80.0f, 10.0f, 0.0f })));
                        SetSelected(newEntity);
                    }
                    if (ImGui::MenuItem("Sky Light"))
                    {
                        auto newEntity = mContext->CreateEntity("Sky Light");
                        newEntity.AddComponent<SkyLightComponent>();
                        SetSelected(newEntity);
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndPopup();
            }

            ImGui::End();

            ImGui::Begin("Properties");

            if (mSelectionContext)
            {
                DrawComponents(mSelectionContext);
            }
        }

        ImGui::End();

#if TODO
        ImGui::Begin("Mesh Debug");
        if (ImGui::CollapsingHeader(mesh->mFilePath.c_str()))
        {
            if (mesh->mIsAnimated)
            {
                if (ImGui::CollapsingHeader("Animation"))
                {
                    if (ImGui::Button(mesh->mAnimationPlaying ? "Pause" : "Play"))
                        mesh->mAnimationPlaying = !mesh->mAnimationPlaying;

                    ImGui::SliderFloat("##AnimationTime", &mesh->mAnimationTime, 0.0f, (float)mesh->mScene->mAnimations[0]->mDuration);
                    ImGui::DragFloat("Time Scale", &mesh->mTimeMultiplier, 0.05f, 0.0f, 10.0f);
                }
            }
        }
        ImGui::End();
#endif
    }

    void SceneHierarchyPanel::DrawEntityNode(Entity entity)
    {
        const char* name = "Entity";
        name = entity.GetComponent<TagComponent>().Tag.c_str();

        ImGuiTreeNodeFlags flags = (entity == mSelectionContext ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        bool opened = ImGui::TreeNodeEx((void*)(uint32_t)entity, flags, name);
        if (ImGui::IsItemClicked())
        {
            mSelectionContext = entity;
            if (mSelectionChangedCallback)
            {
                mSelectionChangedCallback(mSelectionContext);
            }
        }

        bool entityDeleted = false;
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete"))
            {
                entityDeleted = true;
            }

            ImGui::EndPopup();
        }
        if (opened)
        {


            ImGui::TreePop();
        }

        if (entityDeleted)
        {
            mContext->DestroyEntity(entity);
            if (entity == mSelectionContext)
            {
                mSelectionContext = {};
            }
            mEntityDeletedCallback(entity);
        }
    }

    void SceneHierarchyPanel::DrawMeshNode(const Ref<Mesh>& mesh, uint32_t& imguiMeshID)
    {
        static char imguiName[128];
        memset(imguiName, 0, 128);
        sprintf(imguiName, "Mesh##%d", imguiMeshID++);

        // Mesh Hierarchy
        if (ImGui::TreeNode(imguiName))
        {
            auto rootNode = mesh->mScene->mRootNode;
            MeshNodeHierarchy(mesh, rootNode);
            ImGui::TreePop();
        }
    }


    static std::tuple<glm::vec3, glm::quat, glm::vec3> GetTransformDecomposition(const glm::mat4& transform)
    {
        glm::vec3 scale, translation, skew;
        glm::vec4 perspective;
        glm::quat orientation;
        glm::decompose(transform, scale, orientation, translation, skew, perspective);

        return { translation, orientation, scale };
    }

    void SceneHierarchyPanel::MeshNodeHierarchy(const Ref<Mesh>& mesh, aiNode* node, const glm::mat4& parentTransform, uint32_t level)
    {
        glm::mat4 localTransform = AssimpMat4ToMat4(node->mTransformation);
        glm::mat4 transform = parentTransform * localTransform;

        if (ImGui::TreeNode(node->mName.C_Str()))
        {
            {
                auto [translation, rotation, scale] = GetTransformDecomposition(transform);
                ImGui::Text("World Transform");
                ImGui::Text("  Translation: %.2f, %.2f, %.2f", translation.x, translation.y, translation.z);
                ImGui::Text("  Scale: %.2f, %.2f, %.2f", scale.x, scale.y, scale.z);
            }
            {
                auto [translation, rotation, scale] = GetTransformDecomposition(localTransform);
                ImGui::Text("Local Transform");
                ImGui::Text("  Translation: %.2f, %.2f, %.2f", translation.x, translation.y, translation.z);
                ImGui::Text("  Scale: %.2f, %.2f, %.2f", scale.x, scale.y, scale.z);
            }

            for (uint32_t i = 0; i < node->mNumChildren; ++i)
            {
                MeshNodeHierarchy(mesh, node->mChildren[i], transform, level + 1);
            }

            ImGui::TreePop();
        }
    }

    template<typename T, typename UIFunction>
    static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction)
    {
        const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        if (entity.HasComponent<T>())
        {
            auto& component = entity.GetComponent<T>();
            ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
            float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            ImGui::Separator();
            bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
            ImGui::PopStyleVar();
            ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
            if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
            {
                ImGui::OpenPopup("ComponentSettings");
            }

            bool removeComponent = false;

            if (ImGui::BeginPopup("ComponentSettings"))
            {
                if (ImGui::MenuItem("Remove component"))
                {
                    removeComponent = true;
                }

                ImGui::EndPopup();
            }

            if (open)
            {
                uiFunction(component);
                ImGui::TreePop();
            }

            if (removeComponent)
            {
                entity.RemoveComponent<T>();
            }
        }
    }

    static bool DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
    {
        bool modified = false;

        ImGuiIO& io = ImGui::GetIO();
        auto boldFont = io.Fonts->Fonts[0];

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("X", buttonSize))
        {
            values.x = resetValue;
            modified = true;
        }
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        modified |= ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Y", buttonSize))
        {
            values.y = resetValue;
            modified = true;
        }
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        modified |= ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Z", buttonSize))
        {
            values.z = resetValue;
            modified = true;
        }
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        modified |= ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();

        return modified;
    }

    void SceneHierarchyPanel::DrawComponents(Entity entity)
    {
        ImGui::AlignTextToFramePadding();

        auto id = entity.GetComponent<IDComponent>().ID;

        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

        {
            auto& tag = entity.GetComponent<TagComponent>().Tag;
            char buffer[256];
            memset(buffer, 0, 256);
            memcpy(buffer, tag.c_str(), tag.length());
            ImGui::PushItemWidth(contentRegionAvailable.x * 0.5f);
            if (ImGui::InputText("##Tag", buffer, 256))
            {
                tag = std::string(buffer);
            }
            ImGui::PopItemWidth();
        }

        ImGui::SameLine();
        ImGui::TextDisabled("%llx", id);

        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 textSize = ImGui::CalcTextSize("Add Component");
        ImGui::SameLine(contentRegionAvailable.x - (textSize.x + GImGui->Style.FramePadding.y));

        if (ImGui::Button("Add Component"))
        {
            ImGui::OpenPopup("AddComponentPanel");
        }

        if (ImGui::BeginPopup("AddComponentPanel"))
        {
            if (!mSelectionContext.HasComponent<CameraComponent>())
            {
                if (ImGui::Button("Camera"))
                {
                    mSelectionContext.AddComponent<CameraComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!mSelectionContext.HasComponent<MeshComponent>())
            {
                if (ImGui::Button("Mesh"))
                {
                    mSelectionContext.AddComponent<MeshComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!mSelectionContext.HasComponent<DirectionalLightComponent>())
            {
                if (ImGui::Button("Directional Light"))
                {
                    mSelectionContext.AddComponent<DirectionalLightComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!mSelectionContext.HasComponent<SkyLightComponent>())
            {
                if (ImGui::Button("Sky Light"))
                {
                    mSelectionContext.AddComponent<SkyLightComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!mSelectionContext.HasComponent<ScriptComponent>())
            {
                if (ImGui::Button("Script"))
                {
                    mSelectionContext.AddComponent<ScriptComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!mSelectionContext.HasComponent<SpriteRendererComponent>())
            {
                if (ImGui::Button("Sprite Renderer"))
                {
                    mSelectionContext.AddComponent<SpriteRendererComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!mSelectionContext.HasComponent<RigidBody2DComponent>())
            {
                if (ImGui::Button("Rigidbody 2D"))
                {
                    mSelectionContext.AddComponent<RigidBody2DComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!mSelectionContext.HasComponent<BoxCollider2DComponent>())
            {
                if (ImGui::Button("Box Collider 2D"))
                {
                    mSelectionContext.AddComponent<BoxCollider2DComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!mSelectionContext.HasComponent<CircleCollider2DComponent>())
            {
                if (ImGui::Button("Circle Collider 2D"))
                {
                    mSelectionContext.AddComponent<CircleCollider2DComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!mSelectionContext.HasComponent<RigidBodyComponent>())
            {
                if (ImGui::Button("Rigidbody"))
                {
                    mSelectionContext.AddComponent<RigidBodyComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!mSelectionContext.HasComponent<PhysicsMaterialComponent>())
            {
                if (ImGui::Button("Physics Material"))
                {
                    mSelectionContext.AddComponent<PhysicsMaterialComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!mSelectionContext.HasComponent<BoxColliderComponent>())
            {
                if (ImGui::Button("Box Collider"))
                {
                    mSelectionContext.AddComponent<BoxColliderComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!mSelectionContext.HasComponent<SphereColliderComponent>())
            {
                if (ImGui::Button("Sphere Collider"))
                {
                    mSelectionContext.AddComponent<SphereColliderComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!mSelectionContext.HasComponent<SphereColliderComponent>())
            {
                if (ImGui::Button("Capsule Collider"))
                {
                    mSelectionContext.AddComponent<CapsuleColliderComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!mSelectionContext.HasComponent<MeshColliderComponent>())
            {
                if (ImGui::Button("Mesh Collider"))
                {
                    MeshColliderComponent& component = mSelectionContext.AddComponent<MeshColliderComponent>();
                    if (mSelectionContext.HasComponent<MeshComponent>())
                    {
                        component.CollisionMesh = mSelectionContext.GetComponent<MeshComponent>().MeshObj;
                        PhysicsWrappers::CreateTriangleMesh(component);
                    }

                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }

        DrawComponent<TransformComponent>("Transform", entity, [](TransformComponent& component)
            {
                DrawVec3Control("Translation", component.Translation);
                glm::vec3 rotation = glm::degrees(component.Rotation);
                DrawVec3Control("Rotation", rotation);
                component.Rotation = glm::radians(rotation);
                DrawVec3Control("Scale", component.Scale, 1.0f);
            });

        DrawComponent<MeshComponent>("Mesh", entity, [](MeshComponent& mc)
            {
                ImGui::Columns(3);
                ImGui::SetColumnWidth(0, 100);
                ImGui::SetColumnWidth(1, 300);
                ImGui::SetColumnWidth(2, 40);
                ImGui::Text("File Path");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (mc.MeshObj)
                {
                    ImGui::InputText("##meshfilepath", (char*)mc.MeshObj->GetFilePath().c_str(), 256, ImGuiInputTextFlags_ReadOnly);
                }
                else
                {
                    ImGui::InputText("##meshfilepath", (char*)"Null", 256, ImGuiInputTextFlags_ReadOnly);
                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();
                if (ImGui::Button("...##openmesh"))
                {

                    std::string file = Application::Get().OpenFile();
                    if (!file.empty())
                    {
                        mc.MeshObj = Ref<Mesh>::Create(file);
                    }
                }
                ImGui::Columns(1);
            });

        DrawComponent<CameraComponent>("Camera", entity, [](CameraComponent& cc)
            {
                const char* projTypeStrings[] = { "Perspective", "Orthographic" };
                const char* currentProj = projTypeStrings[(int)cc.CameraObj.GetProjectionType()];
                if (ImGui::BeginCombo("Projection", currentProj))
                {

                    for (int type = 0; type < 2; ++type)
                    {
                        bool isSelected = (currentProj == projTypeStrings[type]);
                        if (ImGui::Selectable(projTypeStrings[type], isSelected))
                        {
                            currentProj = projTypeStrings[type];
                            cc.CameraObj.SetProjectionType((SceneCamera::ProjectionType)type);
                        }
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                UI::BeginPropertyGrid();
                if (cc.CameraObj.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
                {
                    float verticalFOV = cc.CameraObj.GetPerspectiveVerticalFOV();
                    if (UI::Property("Vertical FOV", verticalFOV))
                    {
                        cc.CameraObj.SetPerspectiveVerticalFOV(verticalFOV);
                    }

                    float nearClip = cc.CameraObj.GetPerspectiveNearClip();
                    if (UI::Property("Near Clip", nearClip))
                    {
                        cc.CameraObj.SetPerspectiveNearClip(nearClip);
                    }
                    ImGui::SameLine();
                    float farClip = cc.CameraObj.GetPerspectiveFarClip();
                    if (UI::Property("Far Clip", farClip))
                    {
                        cc.CameraObj.SetPerspectiveFarClip(farClip);
                    }
                }
                else if (cc.CameraObj.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
                {
                    float orthoSize = cc.CameraObj.GetOrthographicSize();
                    if (UI::Property("Size", orthoSize))
                    {
                        cc.CameraObj.SetOrthographicSize(orthoSize);
                    }

                    float nearClip = cc.CameraObj.GetOrthographicNearClip();
                    if (UI::Property("Near Clip", nearClip))
                    {
                        cc.CameraObj.SetOrthographicNearClip(nearClip);
                    }
                    ImGui::SameLine();
                    float farClip = cc.CameraObj.GetOrthographicFarClip();
                    if (UI::Property("Far Clip", farClip))
                    {
                        cc.CameraObj.SetOrthographicFarClip(farClip);
                    }
                }
                UI::EndPropertyGrid();
            });

        DrawComponent<SpriteRendererComponent>("Sprite Renderer", entity, [](SpriteRendererComponent& mc)
            {
            });

        DrawComponent<DirectionalLightComponent>("Directional Light", entity, [](DirectionalLightComponent& dlc)
            {
                UI::BeginPropertyGrid();
                UI::PropertyColor("Radiance", dlc.Radiance);
                UI::Property("Intensity", dlc.Intensity);
                UI::Property("Cast Shadows", dlc.CastShadows);
                UI::Property("Soft Shadows", dlc.SoftShadows);
                UI::Property("Source Size", dlc.LightSize);
                UI::EndPropertyGrid();
            });

        DrawComponent<SkyLightComponent>("Sky Light", entity, [](SkyLightComponent& slc)
            {
                ImGui::Columns(3);
                ImGui::SetColumnWidth(0, 100);
                ImGui::SetColumnWidth(1, 300);
                ImGui::SetColumnWidth(2, 40);
                ImGui::Text("File Path");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (!slc.SceneEnvironment.FilePath.empty())
                {
                    ImGui::InputText("##envfilepath", (char*)slc.SceneEnvironment.FilePath.c_str(), 256, ImGuiInputTextFlags_ReadOnly);
                }
                else
                {
                    ImGui::InputText("##envfilepath", (char*)"Empty", 256, ImGuiInputTextFlags_ReadOnly);
                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();
                if (ImGui::Button("...##openenv"))
                {
                    std::string file = Application::Get().OpenFile("*.hdr");
                    if (!file.empty())
                    {
                        slc.SceneEnvironment = Environment::Load(file);
                    }
                }
                ImGui::Columns(1);

                UI::BeginPropertyGrid();
                UI::Property("Intensity", slc.Intensity, 0.01f, 0.0f, 5.0f);
                UI::EndPropertyGrid();
            });

        DrawComponent<ScriptComponent>("Script", entity, [=](ScriptComponent& sc) mutable
            {
                UI::BeginPropertyGrid();
                std::string oldName = sc.ModuleName;
                if (UI::Property("Module Name", sc.ModuleName, !ScriptEngine::ModuleExists(sc.ModuleName)))
                {
                    if (ScriptEngine::ModuleExists(oldName))
                    {
                        ScriptEngine::ShutdownScriptEntity(entity, oldName);
                    }

                    if (ScriptEngine::ModuleExists(sc.ModuleName))
                    {
                        ScriptEngine::InitScriptEntity(entity);
                    }
                }

                // Public Fields
                if (ScriptEngine::ModuleExists(sc.ModuleName))
                {
                    EntityInstanceData& entityInstanceData = ScriptEngine::GetEntityInstanceData(entity.GetSceneID(), id);
                    auto& moduleFieldMap = entityInstanceData.ModuleFieldMap;
                    if (moduleFieldMap.find(sc.ModuleName) != moduleFieldMap.end())
                    {
                        auto& publicFields = moduleFieldMap.at(sc.ModuleName);
                        for (auto& [name, field] : publicFields)
                        {
                            bool isRuntime = mContext->mIsPlaying && field.IsRuntimeAvailable();
                            switch (field.Type)
                            {
                            case FieldType::Int:
                            {
                                int value = isRuntime ? field.GetValue<int>() : field.GetStoredValue<int>();
                                if (UI::Property(field.Name.c_str(), value))
                                {
                                    if (isRuntime)
                                    {
                                        field.SetValue(value);
                                    }
                                    else
                                    {
                                        field.SetStoredValue(value);
                                    }
                                }
                                break;
                            }
                            case FieldType::Float:
                            {
                                float value = isRuntime ? field.GetValue<float>() : field.GetStoredValue<float>();
                                if (UI::Property(field.Name.c_str(), value, 0.2f))
                                {
                                    if (isRuntime)
                                    {
                                        field.SetValue(value);
                                    }
                                    else
                                    {
                                        field.SetStoredValue(value);
                                    }
                                }
                                break;
                            }
                            case FieldType::Vec2:
                            {
                                glm::vec2 value = isRuntime ? field.GetValue<glm::vec2>() : field.GetStoredValue<glm::vec2>();
                                if (UI::Property(field.Name.c_str(), value, 0.2f))
                                {
                                    if (isRuntime)
                                    {
                                        field.SetValue(value);
                                    }
                                    else
                                    {
                                        field.SetStoredValue(value);
                                    }
                                }
                                break;
                            }
                            case FieldType::Vec3:
                            {
                                glm::vec3 value = isRuntime ? field.GetValue<glm::vec3>() : field.GetStoredValue<glm::vec3>();
                                if (UI::Property(field.Name.c_str(), value, 0.2f))
                                {
                                    if (isRuntime)
                                    {
                                        field.SetValue(value);
                                    }
                                    else
                                    {
                                        field.SetStoredValue(value);
                                    }
                                }
                                break;
                            }
                            case FieldType::Vec4:
                            {
                                glm::vec4 value = isRuntime ? field.GetValue<glm::vec4>() : field.GetStoredValue<glm::vec4>();
                                if (UI::Property(field.Name.c_str(), value, 0.2f))
                                {
                                    if (isRuntime)
                                    {
                                        field.SetValue(value);
                                    }
                                    else
                                    {
                                        field.SetStoredValue(value);
                                    }
                                }
                                break;
                            }
                            }
                        }
                    }
                }

                UI::EndPropertyGrid();
#if TODO
                if (ImGui::Button("Run Script"))
                {
                    ScriptEngine::OnCreateEntity(entity);
                }
#endif
            });

        DrawComponent<RigidBody2DComponent>("Rigidbody 2D", entity, [](RigidBody2DComponent& rb2dc)
            {
                // Rigidbody2D Type
                const char* rb2dTypeStrings[] = { "Static", "Dynamic", "Kinematic" };
                const char* currentType = rb2dTypeStrings[(int)rb2dc.BodyType];

                ImGui::TextUnformatted("Type");
                ImGui::SameLine();
                if (ImGui::BeginCombo("##TypeSelection", currentType))
                {
                    for (int type = 0; type < 3; ++type)
                    {
                        bool isSelected = (currentType == rb2dTypeStrings[type]);
                        if (ImGui::Selectable(rb2dTypeStrings[type], isSelected))
                        {
                            currentType = rb2dTypeStrings[type];
                            rb2dc.BodyType = (RigidBody2DComponent::Type)type;
                        }
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                if (rb2dc.BodyType == RigidBody2DComponent::Type::Dynamic)
                {
                    UI::BeginPropertyGrid();
                    UI::Property("Fixed Rotation", rb2dc.FixedRotation);
                    UI::EndPropertyGrid();
                }
            });

        DrawComponent<BoxCollider2DComponent>("Box Collider 2D", entity, [](BoxCollider2DComponent& bc2dc)
            {
                UI::BeginPropertyGrid();

                UI::Property("Offset", bc2dc.Offset);
                UI::Property("Size", bc2dc.Size);
                UI::Property("Density", bc2dc.Density);
                UI::Property("Friction", bc2dc.Friction);

                UI::EndPropertyGrid();
            });

        DrawComponent<CircleCollider2DComponent>("Circle Collider 2D", entity, [](CircleCollider2DComponent& cc2dc)
            {
                UI::BeginPropertyGrid();

                UI::Property("Offset", cc2dc.Offset);
                UI::Property("Radius", cc2dc.Radius);
                UI::Property("Density", cc2dc.Density);
                UI::Property("Friction", cc2dc.Friction);

                UI::EndPropertyGrid();
            });

        DrawComponent<RigidBodyComponent>("Rigidbody", entity, [](RigidBodyComponent& rbc)
            {
                const char* rbTypeStrings[] = { "Static", "Dynamic" };
                const char* currentType = rbTypeStrings[(int)rbc.BodyType];
                if (ImGui::BeginCombo("Type", currentType))
                {
                    for (int type = 0; type < 2; ++type)
                    {
                        bool isSelected = (currentType == rbTypeStrings[type]);
                        if (ImGui::Selectable(rbTypeStrings[type], isSelected))
                        {
                            currentType = rbTypeStrings[type];
                            rbc.BodyType = (RigidBodyComponent::Type)type;
                        }
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                if (!PhysicsLayerManager::IsLayerValid(rbc.Layer))
                {
                    rbc.Layer = 0;
                }

                uint32_t currentLayer = rbc.Layer;
                const PhysicsLayer& layerInfo = PhysicsLayerManager::GetLayer(currentLayer);

                ImGui::TextUnformatted("Layer");
                ImGui::SameLine();
                if (ImGui::BeginCombo("##LayerSelection", layerInfo.Name.c_str()))
                {
                    for (const auto& layer : PhysicsLayerManager::GetLayers())
                    {
                        bool isSelected = (currentLayer == layer.ID);
                        if (ImGui::Selectable(layer.Name.c_str(), isSelected))
                        {
                            currentLayer = layer.ID;
                            rbc.Layer = layer.ID;
                        }
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                if (rbc.BodyType == RigidBodyComponent::Type::Dynamic)
                {
                    UI::BeginPropertyGrid();
                    UI::Property("Mass", rbc.Mass);
                    UI::Property("Linear Drag", rbc.LinearDrag);
                    UI::Property("Angular Drag", rbc.AngularDrag);
                    UI::Property("Disable Gravity", rbc.DisableGravity);
                    UI::Property("Is Kinematic", rbc.IsKinematic);
                    UI::EndPropertyGrid();

                    if (ImGui::TreeNode("Constraints", "Constraints"))
                    {
                        UI::BeginPropertyGrid();

                        UI::BeginCheckboxGroup("Freeze Position");
                        UI::PropertyCheckboxGroup("X", rbc.LockPositionX);
                        UI::PropertyCheckboxGroup("Y", rbc.LockPositionY);
                        UI::PropertyCheckboxGroup("Z", rbc.LockPositionZ);
                        UI::EndCheckboxGroup();

                        UI::BeginCheckboxGroup("Freeze Rotation");
                        UI::PropertyCheckboxGroup("X", rbc.LockRotationX);
                        UI::PropertyCheckboxGroup("Y", rbc.LockRotationY);
                        UI::PropertyCheckboxGroup("Z", rbc.LockRotationZ);
                        UI::EndCheckboxGroup();

                        UI::EndPropertyGrid();

                        ImGui::TreePop();
                    }
                }
            });

        DrawComponent<PhysicsMaterialComponent>("Physics Material", entity, [](PhysicsMaterialComponent& pmc)
            {
                UI::BeginPropertyGrid();

                UI::Property("Static Friction", pmc.StaticFriction);
                UI::Property("Dynamic Friction", pmc.DynamicFriction);
                UI::Property("Bounciness", pmc.Bounciness);

                UI::EndPropertyGrid();
            });

        DrawComponent<BoxColliderComponent>("Box Collider", entity, [](BoxColliderComponent& bcc)
            {
                UI::BeginPropertyGrid();

                if (UI::Property("Size", bcc.Size))
                {
                    bcc.DebugMesh = MeshFactory::CreateBox(bcc.Size);
                }
                UI::Property("Is Trigger", bcc.IsTrigger);

                UI::EndPropertyGrid();
            });

        DrawComponent<SphereColliderComponent>("Sphere Collider", entity, [](SphereColliderComponent& scc)
            {
                UI::BeginPropertyGrid();

                if (UI::Property("Radius", scc.Radius))
                {
                    scc.DebugMesh = MeshFactory::CreateSphere(scc.Radius);
                }
                UI::Property("Is Trigger", scc.IsTrigger);

                UI::EndPropertyGrid();
            });

        DrawComponent<CapsuleColliderComponent>("Capsule Collider", entity, [](CapsuleColliderComponent& ccc)
            {
                UI::BeginPropertyGrid();

                bool changed = false;
                if (UI::Property("Radius", ccc.Radius))
                {
                    changed = true;
                }
                if (UI::Property("Height", ccc.Height))
                {
                    changed = true;
                }
                if (changed)
                {
                    ccc.DebugMesh = MeshFactory::CreateCapsule(ccc.Radius, ccc.Height);
                }
                UI::Property("Is Trigger", ccc.IsTrigger);

                UI::EndPropertyGrid();
            });

        DrawComponent<MeshColliderComponent>("Mesh Collider", entity, [&](MeshColliderComponent& mcc)
            {
                if (mcc.OverrideMesh)
                {
                    ImGui::Columns(3);
                    ImGui::SetColumnWidth(0, 100);
                    ImGui::SetColumnWidth(1, 300);
                    ImGui::SetColumnWidth(2, 40);
                    ImGui::Text("File Path");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (mcc.CollisionMesh)
                    {
                        ImGui::InputText("##meshfilepath", (char*)mcc.CollisionMesh->GetFilePath().c_str(), 256, ImGuiInputTextFlags_ReadOnly);
                    }
                    else
                    {
                        ImGui::InputText("##meshfilepath", (char*)"Null", 256, ImGuiInputTextFlags_ReadOnly);
                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();
                    if (ImGui::Button("...##openmesh"))
                    {
                        std::string file = Application::Get().OpenFile();
                        if (!file.empty())
                        {
                            mcc.CollisionMesh = Ref<Mesh>::Create(file);
                            if (mcc.IsConvex)
                            {
                                PhysicsWrappers::CreateConvexMesh(mcc, glm::vec3(1.0f), true);
                            }
                            else
                            {
                                PhysicsWrappers::CreateTriangleMesh(mcc, glm::vec3(1.0f), true);
                            }
                        }
                    }
                    ImGui::Columns(1);
                }

                UI::BeginPropertyGrid();
                if (UI::Property("Is Convex", mcc.IsConvex))
                {
                    if (mcc.IsConvex)
                    {
                        PhysicsWrappers::CreateConvexMesh(mcc, glm::vec3(1.0f), true);
                    }
                    else
                    {
                        PhysicsWrappers::CreateTriangleMesh(mcc, glm::vec3(1.0f), true);
                    }
                }

                UI::Property("Is Trigger", mcc.IsTrigger);
                if (UI::Property("Override Mesh", mcc.OverrideMesh))
                {
                    if (!mcc.OverrideMesh && entity.HasComponent<MeshComponent>())
                    {
                        mcc.CollisionMesh = entity.GetComponent<MeshComponent>().MeshObj;

                        if (mcc.IsConvex)
                        {
                            PhysicsWrappers::CreateConvexMesh(mcc, glm::vec3(1.0f), true);
                        }
                        else
                        {
                            PhysicsWrappers::CreateTriangleMesh(mcc, glm::vec3(1.0f), true);
                        }
                    }
                }
                UI::EndPropertyGrid();
            });
    }
}