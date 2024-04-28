#include "SceneHierarchyPanel.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <glm/gtc/type_ptr.hpp>

#include "NotRed/UI/ImGuiTools.h"

#include "NotRed/Scripting/ScriptEngine.h"

#include "NotRed/Scene/Components.h"

namespace NR
{
    SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
    {
        SetContext(context);
    }

    void SceneHierarchyPanel::SetContext(const Ref<Scene>& context)
    {
        mContext = context;
        mSelectionContext = {};
    }

    void SceneHierarchyPanel::ImGuiRender()
    {
        ImGui::Begin("Scene Hierarchy");

        mContext->mRegistry.view<TagComponent>().each([&](auto entityID, auto)
            {
                Entity entity{ entityID, mContext.get() };
                DrawNode(entity);
            });

        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
        {
            mSelectionContext = {};
        }

        if (ImGui::BeginPopupContextWindow(0, 1 | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("Create Entity"))
            {
                mContext->CreateEntity();
            }

            ImGui::EndPopup();
        }

        ImGui::End();
        ImGui::Begin("Properties");

        if (mSelectionContext)
        {
            DrawComponents(mSelectionContext);
        }

        ImGui::End();
    }

    void SceneHierarchyPanel::SetSelectedEntity(Entity entity)
    {
        mSelectionContext = entity;
    }

    void SceneHierarchyPanel::DrawNode(Entity& entity)
    {
        auto& tag = entity.GetComponent<TagComponent>().Tag;

        ImGuiTreeNodeFlags flags = ((mSelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0);
        flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

        bool isOpened = ImGui::TreeNodeEx((void*)(uint32_t)entity, flags, tag.c_str());
        if (ImGui::IsItemClicked())
        {
            mSelectionContext = entity;
        }

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete Entity"))
            {
                if (mSelectionContext == entity)
                {
                    mSelectionContext = {};
                }
                mContext->RemoveEntity(entity);
            }

            ImGui::EndPopup();
        }

        if (isOpened)
        {
            ImGui::TreePop();
        }
    }

    static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
    {
        ImGuiIO& io = ImGui::GetIO();
        auto italicFont = io.Fonts->Fonts[0];

        ImGui::PushID(label.c_str());

        ImGui::PushFont(italicFont);
        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.95f, 0.25f, 0.35f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.7f, 0.05f, 0.07f, 1.0f });
        if (ImGui::Button("X", buttonSize))
        {
            values.x = resetValue;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.25f, 0.75f, 0.25f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.35f, 0.85f, 0.35f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.6f, 0.1f, 1.0f });
        if (ImGui::Button("Y", buttonSize))
        {
            values.y = resetValue;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.15f, 0.1f, 0.8f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.2f, 1.0f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.07f, 0.05f, 0.7f, 1.0f });
        if (ImGui::Button("Z", buttonSize))
        {
            values.z = resetValue;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);
        ImGui::PopFont();

        ImGui::PopID();
    }

    template<typename T, typename UIFunc>
    static void DrawComponent(const std::string& name, Entity entity, UIFunc function, bool destroyableComponent = true)
    {
        if (entity.HasComponent<T>())
        {
            const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed
                | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap
                | ImGuiTreeNodeFlags_FramePadding;

            ImVec2 contentRegAvail = ImGui::GetContentRegionAvail();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
            float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            ImGui::Separator();
            bool openPanel = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
            ImGui::PopStyleVar();

            if (destroyableComponent)
            {
                ImGui::SameLine(contentRegAvail.x - lineHeight * 0.5f);
                if (destroyableComponent && ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
                {
                    ImGui::OpenPopup("ComponentSettings");
                }
            }

            bool deleteSelected = false;
            if (ImGui::BeginPopup("ComponentSettings"))
            {
                if (ImGui::MenuItem("Remove Component"))
                {
                    deleteSelected = true;
                }

                ImGui::EndPopup();
            }

            if (openPanel)
            {
                function(entity.GetComponent<T>());
                ImGui::TreePop();
            }

            if (deleteSelected)
            {
                entity.RemoveComponent<T>();
                deleteSelected = false;
            }
        }
    }

    void SceneHierarchyPanel::DrawComponents(Entity& entity)
    {
        {
            auto& tag = entity.GetComponent<TagComponent>().Tag;

            char buffer[250];
            memset(buffer, 0, sizeof(buffer));
            strncpy_s(buffer, sizeof(buffer), tag.c_str(), sizeof(buffer));
            if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
            {
                tag = std::string(buffer);
            }
        }

        ImGui::SameLine();
        ImGui::PushItemWidth(-1);

        if (ImGui::Button("Add Component"))
        {
            ImGui::OpenPopup("AddComponent");
        }

        if (ImGui::BeginPopup("AddComponent"))
        {
            DisplayAddComponent<CameraComponent>("Camera");
            DisplayAddComponent<ScriptComponent>("Script");
            DisplayAddComponent<SpriteRendererComponent>("Sprite Renderer");
            DisplayAddComponent<CircleRendererComponent>("Circle Renderer");
            DisplayAddComponent<TextComponent>("Text Component");
            DisplayAddComponent<Rigidbody2DComponent>("Rigidbody 2D");
            DisplayAddComponent<BoxCollider2DComponent>("Box Collider 2D");
            DisplayAddComponent<CircleCollider2DComponent>("Circle Collider 2D");

            ImGui::EndPopup();
        }

        ImGui::PopItemWidth();

        DrawComponent<TransformComponent>("Transform", entity, [](TransformComponent& component)
            {
                DrawVec3Control("Position", component.Translation);
                glm::vec3 rotation = glm::degrees(component.Rotation);
                DrawVec3Control("Rotation", rotation);
                component.Rotation = glm::radians(rotation);
                DrawVec3Control("Scale", component.Scale, 1.0f);
            }, false);

        DrawComponent<CameraComponent>("Camera", entity, [](CameraComponent& component)
            {
                auto& camera = component.Camera;
                const char* projectionTypes[] = { "Perspective", "Orthographic" };
                int currentProjection = (int)camera.GetProjectionType();

                ImGui::Checkbox("Primary Camera", &component.IsPrimary);
                ImGui::Checkbox("Fixed Aspect Ratio", &component.HasFixedAspectRatio);

                if (ImGui::Combo("Projection", &currentProjection, projectionTypes, 2))
                {
                    camera.SetProjectionType((SceneCamera::ProjectionType)currentProjection);
                }

                if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
                {
                    float fov = glm::degrees(camera.GetFOV());
                    if (ImGui::DragFloat("FOV", &fov, 0.5f))
                    {
                        camera.SetFOV(glm::radians(fov));
                    }

                    float perspectiveNear = camera.GetNearClip();
                    if (ImGui::DragFloat("Near Clip", &perspectiveNear, 0.5f))
                    {
                        camera.SetNearClip(perspectiveNear);
                    }

                    float perspectiveFar = camera.GetFarClip();
                    if (ImGui::DragFloat("Far Clip", &perspectiveFar, 0.5f))
                    {
                        camera.SetFarClip(perspectiveFar);
                    }
                }
                else
                {
                    float orthoSize = camera.GetFOV();
                    if (ImGui::DragFloat("Size", &orthoSize, 0.5f))
                    {
                        camera.SetFOV(orthoSize);
                    }

                    float orthoNear = camera.GetNearClip();
                    if (ImGui::DragFloat("Near Clip", &orthoNear, 0.5f))
                    {
                        camera.SetNearClip(orthoNear);
                    }

                    float orthoFar = camera.GetFarClip();
                    if (ImGui::DragFloat("Far Clip", &orthoFar, 0.5f))
                    {
                        camera.SetFarClip(orthoFar);
                    }

                }
            });

        DrawComponent<ScriptComponent>("Script", entity, [entity, scene = mContext](ScriptComponent& component) mutable
            {
                bool scriptClassExists = ScriptEngine::EntityClassExists(component.ClassName);

                static char buffer[64];
                strcpy_s(buffer, sizeof(buffer), component.ClassName.c_str());

                UI::StyleColor textColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.3f, 1.0f), !scriptClassExists);

                if (ImGui::InputText("Class", buffer, sizeof(buffer)))
                {
                    component.ClassName = buffer;
                    return;
                }

                bool sceneRunning = scene->IsRunning();
                if (sceneRunning)
                {
                    Ref<ScriptClassInstance> scriptInstance = ScriptEngine::GetEntityScriptInstance(entity.GetUUID());
                    if (scriptInstance)
                    {
                        const auto& fields = scriptInstance->GetScriptClass()->GetFields();

                        for (const auto& [name, field] : fields)
                        {
                            if (field.Type == ScriptFieldType::Float)
                            {
                                float data = scriptInstance->GetFieldValue<float>(name);
                                if (ImGui::DragFloat(name.c_str(), &data))
                                {
                                    scriptInstance->SetFieldValue(name, data);
                                }
                            }
                        }
                    }
                }
                else if (scriptClassExists)
                {
                    Ref<ScriptClass> entityClass = ScriptEngine::GetEntityClass(component.ClassName);
                    const auto& fields = entityClass->GetFields();

                    auto& entityFields = ScriptEngine::GetScriptFieldMap(entity);
                    for (const auto& [name, field] : fields)
                    {
                        if (entityFields.find(name) != entityFields.end())
                        {
                            ScriptFieldInstance& scriptField = entityFields.at(name);

                            if (field.Type == ScriptFieldType::Float)
                            {
                                float data = scriptField.GetValue<float>();
                                if (ImGui::DragFloat(name.c_str(), &data))
                                {
                                    scriptField.SetValue(data);
                                }
                            }
                        }
                        else if (field.Type == ScriptFieldType::Float)
                        {
                            float data = 0.0f;
                            if (ImGui::DragFloat(name.c_str(), &data))
                            {
                                ScriptFieldInstance& fieldInstance = entityFields[name];
                                fieldInstance.Field = field;
                                fieldInstance.SetValue(data);
                            }
                        }
                    }

                }
            });

        DrawComponent<SpriteRendererComponent>("Sprite Renderer", entity, [](SpriteRendererComponent& component)
            {
                ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
            });

        DrawComponent<CircleRendererComponent>("Circle Renderer", entity, [](CircleRendererComponent& component)
            {
                ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
                ImGui::DragFloat("Thickness", &component.Thickness, 0.025f, 0.0f, 1.0f);
            });

        DrawComponent<TextComponent>("Text Renderer", entity, [](TextComponent& component)
            {
                ImGui::InputTextMultiline("Text String", &component.TextString);
                ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
                ImGui::DragFloat("Line Spacing", &component.LineSpacing, 0.025f);
            });

        DrawComponent<Rigidbody2DComponent>("Rigidbody 2D", entity, [](Rigidbody2DComponent& component)
            {
                const char* bodyTypeStrings[] = { "Static", "Kinematic", "Dynamic" };
                int currentBodyTypeIndex = (int)component.Type;

                if (ImGui::Combo("Body Type", &currentBodyTypeIndex, bodyTypeStrings, 3))
                {
                    component.Type = (Rigidbody2DComponent::BodyType)currentBodyTypeIndex;
                }

                ImGui::Checkbox("Fixed Rotation", &component.FixedRotation);
                ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("Restitution Threshold", &component.RestitutionThreshold, 0.01f, 0.0f);
            });

        DrawComponent<BoxCollider2DComponent>("Box Collider 2D", entity, [](BoxCollider2DComponent& component)
            {
                ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
                ImGui::DragFloat2("Size", glm::value_ptr(component.Size));
            });

        DrawComponent<CircleCollider2DComponent>("Circle Collider 2D", entity, [](CircleCollider2DComponent& component)
            {
                ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
                ImGui::DragFloat("Radius", &component.Radius);
            });
    }

    template<typename T>
    void SceneHierarchyPanel::DisplayAddComponent(const std::string& entryName)
    {
        if (!mSelectionContext.HasComponent<T>() && ImGui::MenuItem(entryName.c_str()))
        {
            mSelectionContext.AddComponent<T>();
            ImGui::CloseCurrentPopup();
        }
    }
}
