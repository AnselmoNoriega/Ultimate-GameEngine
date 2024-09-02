#include "nrpch.h"
#include "SceneHierarchyPanel.h"

#include <imgui.h>
#include <imgui/imgui_internal.h>

#include <assimp/scene.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "NotRed/Core/Application.h"
#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Physics/PhysicsLayer.h"
#include "NotRed/Script/ScriptEngine.h"

#include "NotRed/Physics/PhysicsWrappers.h"
#include "NotRed/Renderer/MeshFactory.h"

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
                if (ImGui::MenuItem("Create Empty Entity"))
                {
                    mContext->CreateEntity("Entity");
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

    static int sUIContextID = 0;
    static uint32_t sCounter = 0;
    static char sIDBuffer[16];

    static void PushID()
    {
        ImGui::PushID(sUIContextID++);
        sCounter = 0;
    }

    static void PopID()
    {
        ImGui::PopID();
        --sUIContextID;
    }

    static void BeginPropertyGrid()
    {
        PushID();
        ImGui::Columns(2);
    }

    static bool Property(const char* label, std::string& value, bool error = false)
    {
        bool modified = false;

        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        char buffer[256];
        strcpy(buffer, value.c_str());

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        itoa(sCounter++, sIDBuffer + 2, 16);

        if (error)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 2.0f));
        }

        if (ImGui::InputText(sIDBuffer, buffer, 256))
        {
            value = buffer;
            modified = true;
        }

        if (error)
        {
            ImGui::PopStyleColor();
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return modified;
    }

    static void Property(const char* label, const char* value)
    {
        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        itoa(sCounter++, sIDBuffer + 2, 16);

        ImGui::BeginDisabled(true);
        ImGui::InputText(sIDBuffer, (char*)value, 256);
        ImGui::EndDisabled();

        ImGui::PopItemWidth();
        ImGui::NextColumn();
    }

    static bool Property(const char* label, bool& value)
    {
        bool modified = false;

        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        itoa(sCounter++, sIDBuffer + 2, 16);
        if (ImGui::Checkbox(sIDBuffer, &value))
            modified = true;

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return modified;
    }

    static bool Property(const char* label, int& value)
    {
        bool modified = false;

        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        itoa(sCounter++, sIDBuffer + 2, 16);
        if (ImGui::DragInt(sIDBuffer, &value))
        {
            modified = true;
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return modified;
    }

    static bool Property(const char* label, float& value, float delta = 0.1f)
    {
        bool modified = false;

        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        itoa(sCounter++, sIDBuffer + 2, 16);
        if (ImGui::DragFloat(sIDBuffer, &value, delta))
        {
            modified = true;
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return modified;
    }

    static bool Property(const char* label, glm::vec2& value, float delta = 0.1f)
    {
        bool modified = false;

        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        itoa(sCounter++, sIDBuffer + 2, 16);
        if (ImGui::DragFloat2(sIDBuffer, glm::value_ptr(value), delta))
        {
            modified = true;
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return modified;
    }


    static bool Property(const char* label, glm::vec3& value, float delta = 0.1f)
    {
        bool modified = false;

        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        itoa(sCounter++, sIDBuffer + 2, 16);
        if (ImGui::DragFloat3(sIDBuffer, glm::value_ptr(value), delta))
        {
            modified = true;
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return modified;
    }

    static bool Property(const char* label, glm::vec4& value, float delta = 0.1f)
    {
        bool modified = false;

        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        itoa(sCounter++, sIDBuffer + 2, 16);
        if (ImGui::DragFloat4(sIDBuffer, glm::value_ptr(value), delta))
        {
            modified = true;
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return modified;
    }

    static void EndPropertyGrid()
    {
        ImGui::Columns(1);
        PopID();
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
            ImGui::EndPopup();
        }

        DrawComponent<TransformComponent>("Transform", entity, [](auto& component)
            {
                auto [translation, rotationQuat, scale] = GetTransformDecomposition(component);

                bool updateTransform = false;
                updateTransform |= DrawVec3Control("Translation", translation);
                glm::vec3 rotation = glm::degrees(glm::eulerAngles(rotationQuat));
                updateTransform |= DrawVec3Control("Rotation", rotation);
                updateTransform |= DrawVec3Control("Scale", scale, 1.0f);

                if (updateTransform)
                {
                    component.Transform = glm::translate(glm::mat4(1.0f), translation) *
                        glm::toMat4(glm::quat(glm::radians(rotation))) *
                        glm::scale(glm::mat4(1.0f), scale);
                }
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

                BeginPropertyGrid();
                if (cc.CameraObj.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
                {
                    float verticalFOV = cc.CameraObj.GetPerspectiveVerticalFOV();
                    if (Property("Vertical FOV", verticalFOV))
                    {
                        cc.CameraObj.SetPerspectiveVerticalFOV(verticalFOV);
                    }

                    float nearClip = cc.CameraObj.GetPerspectiveNearClip();
                    if (Property("Near Clip", nearClip))
                    {
                        cc.CameraObj.SetPerspectiveNearClip(nearClip);
                    }
                    ImGui::SameLine();
                    float farClip = cc.CameraObj.GetPerspectiveFarClip();
                    if (Property("Far Clip", farClip))
                    {
                        cc.CameraObj.SetPerspectiveFarClip(farClip);
                    }
                }
                else if (cc.CameraObj.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
                {
                    float orthoSize = cc.CameraObj.GetOrthographicSize();
                    if (Property("Size", orthoSize))
                    {
                        cc.CameraObj.SetOrthographicSize(orthoSize);
                    }

                    float nearClip = cc.CameraObj.GetOrthographicNearClip();
                    if (Property("Near Clip", nearClip))
                    {
                        cc.CameraObj.SetOrthographicNearClip(nearClip);
                    }
                    ImGui::SameLine();
                    float farClip = cc.CameraObj.GetOrthographicFarClip();
                    if (Property("Far Clip", farClip))
                    {
                        cc.CameraObj.SetOrthographicFarClip(farClip);
                    }
                }
                EndPropertyGrid();
            });

        DrawComponent<SpriteRendererComponent>("Sprite Renderer", entity, [](SpriteRendererComponent& mc)
            {
            });

        DrawComponent<ScriptComponent>("Script", entity, [=](ScriptComponent& sc) mutable
            {
                BeginPropertyGrid();
                std::string oldName = sc.ModuleName;
                if (Property("Module Name", sc.ModuleName, !ScriptEngine::ModuleExists(sc.ModuleName)))
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
                                if (Property(field.Name.c_str(), value))
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
                                if (Property(field.Name.c_str(), value, 0.2f))
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
                                if (Property(field.Name.c_str(), value, 0.2f))
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
                                if (Property(field.Name.c_str(), value, 0.2f))
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
                                if (Property(field.Name.c_str(), value, 0.2f))
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

                EndPropertyGrid();
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
                    BeginPropertyGrid();
                    Property("Fixed Rotation", rb2dc.FixedRotation);
                    EndPropertyGrid();
                }
            });

        DrawComponent<BoxCollider2DComponent>("Box Collider 2D", entity, [](BoxCollider2DComponent& bc2dc)
            {
                BeginPropertyGrid();

                Property("Offset", bc2dc.Offset);
                Property("Size", bc2dc.Size);
                Property("Density", bc2dc.Density);
                Property("Friction", bc2dc.Friction);

                EndPropertyGrid();
            });

        DrawComponent<CircleCollider2DComponent>("Circle Collider 2D", entity, [](CircleCollider2DComponent& cc2dc)
            {
                BeginPropertyGrid();

                Property("Offset", cc2dc.Offset);
                Property("Radius", cc2dc.Radius);
                Property("Density", cc2dc.Density);
                Property("Friction", cc2dc.Friction);

                EndPropertyGrid();
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
                    BeginPropertyGrid();
                    Property("Mass", rbc.Mass);
                    Property("Is Kinematic", rbc.IsKinematic);
                    EndPropertyGrid();

                    if (ImGui::TreeNode("RigidBodyConstraints", "Constraints"))
                    {
                        BeginPropertyGrid();
                        Property("Position: X", rbc.LockPositionX);
                        Property("Position: Y", rbc.LockPositionY);
                        Property("Position: Z", rbc.LockPositionZ);

                        Property("Rotation: X", rbc.LockRotationX);
                        Property("Rotation: Y", rbc.LockRotationY);
                        Property("Rotation: Z", rbc.LockRotationZ);
                        EndPropertyGrid();

                        ImGui::TreePop();
                    }
                }
            });

        DrawComponent<PhysicsMaterialComponent>("Physics Material", entity, [](PhysicsMaterialComponent& pmc)
            {
                BeginPropertyGrid();

                Property("Static Friction", pmc.StaticFriction);
                Property("Dynamic Friction", pmc.DynamicFriction);
                Property("Bounciness", pmc.Bounciness);

                EndPropertyGrid();
            });

        DrawComponent<BoxColliderComponent>("Box Collider", entity, [](BoxColliderComponent& bcc)
            {
                BeginPropertyGrid();

                if (Property("Size", bcc.Size))
                {
                    bcc.DebugMesh = MeshFactory::CreateBox(bcc.Size);
                }
                Property("Is Trigger", bcc.IsTrigger);

                EndPropertyGrid();
            });

        DrawComponent<SphereColliderComponent>("Sphere Collider", entity, [](SphereColliderComponent& scc)
            {
                BeginPropertyGrid();

                if (Property("Radius", scc.Radius))
                {
                    scc.DebugMesh = MeshFactory::CreateSphere(scc.Radius);
                }
                Property("Is Trigger", scc.IsTrigger);

                EndPropertyGrid();
            });

        DrawComponent<CapsuleColliderComponent>("Capsule Collider", entity, [](CapsuleColliderComponent& ccc)
            {
                BeginPropertyGrid();

                bool changed = false;
                if (Property("Radius", ccc.Radius))
                {
                    changed = true;
                }
                if (Property("Height", ccc.Height))
                {
                    changed = true;
                }
                if (changed)
                {
                    ccc.DebugMesh = MeshFactory::CreateCapsule(ccc.Radius, ccc.Height);
                }
                Property("Is Trigger", ccc.IsTrigger);

                EndPropertyGrid();
            });

        DrawComponent<MeshColliderComponent>("Mesh Collider", entity, [](MeshColliderComponent& mcc)
            {
                BeginPropertyGrid();
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
                        PhysicsWrappers::CreateConvexMesh(mcc);
                    }
                }

                Property("Is Trigger", mcc.IsTrigger);
                EndPropertyGrid();
            });
                            }
                        }