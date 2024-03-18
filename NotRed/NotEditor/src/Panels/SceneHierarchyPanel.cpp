#include "SceneHierarchyPanel.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>

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

            if (ImGui::Button("Add Component"))
            {
                ImGui::OpenPopup("AddComponent");
            }

            if (ImGui::BeginPopup("AddComponent"))
            {
                if (ImGui::MenuItem("Camera"))
                {
                    mSelectionContext.AddComponent<CameraComponent>();
                    ImGui::CloseCurrentPopup();
                }

                if (ImGui::MenuItem("Sprite Renderer"))
                {
                    mSelectionContext.AddComponent<SpriteRendererComponent>();
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }

        ImGui::End();
    }

    void SceneHierarchyPanel::DrawNode(Entity& entity)
    {
        auto& tag = entity.GetComponent<TagComponent>().Tag;

        ImGuiTreeNodeFlags flags = ((mSelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
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

        ImGui::PopID();
    }

    void SceneHierarchyPanel::DrawComponents(Entity& entity)
    {
        {
            auto& tag = entity.GetComponent<TagComponent>().Tag;

            char buffer[250];
            memset(buffer, 0, sizeof(buffer));
            strcpy_s(buffer, sizeof(buffer), tag.c_str());
            if (ImGui::InputText("Tag", buffer, sizeof(buffer)))
            {
                tag = std::string(buffer);
            }
        }

        const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap;

        if (ImGui::TreeNodeEx((void*)typeid(TransformComponent).hash_code(), treeNodeFlags, "Transform"))
        {
            auto& tc = entity.GetComponent<TransformComponent>();
            DrawVec3Control("Position", tc.Translation);
            glm::vec3 rotation = glm::degrees(tc.Rotation);
            DrawVec3Control("Rotation", rotation);
            tc.Rotation = glm::radians(rotation);
            DrawVec3Control("Scale", tc.Scale, 1.0f);

            ImGui::TreePop();
        }

        if (entity.HasComponent<CameraComponent>())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
            mOpenPanel = ImGui::TreeNodeEx((void*)typeid(CameraComponent).hash_code(), treeNodeFlags, "Camera");

            ImGui::SameLine(ImGui::GetWindowWidth() - 25.0f);
            if (ImGui::Button("+", ImVec2{ 20, 20 }))
            {
                ImGui::OpenPopup("ComponentSettings");
            }
            ImGui::PopStyleVar();

            if (ImGui::BeginPopup("ComponentSettings"))
            {
                if (ImGui::MenuItem("Remove Component"))
                {
                    mDeleteSelected = true;
                }

                ImGui::EndPopup();
            }

            if (mOpenPanel)
            {
                auto& cameraComponent = entity.GetComponent<CameraComponent>();
                auto& camera = entity.GetComponent<CameraComponent>().Camera;
                const char* projectionTypes[] = { "Perspective", "Orthographic" };
                int currentProjection = (int)camera.GetProjectionType();

                ImGui::Checkbox("Primary Camera", &cameraComponent.IsPrimary);
                ImGui::Checkbox("Fixed Aspect Ratio", &cameraComponent.HasFixedAspectRatio);

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

                ImGui::TreePop();
            }


            if (mDeleteSelected)
            {
                entity.RemoveComponent<CameraComponent>();
                mDeleteSelected = false;
            }
        }

        if (entity.HasComponent<SpriteRendererComponent>())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
            mOpenPanel = ImGui::TreeNodeEx((void*)typeid(SpriteRendererComponent).hash_code(), treeNodeFlags, "Sprite Renderer");

            ImGui::SameLine(ImGui::GetWindowWidth() - 25.0f);
            if (ImGui::Button("+", ImVec2{ 20, 20 }))
            {
                ImGui::OpenPopup("ComponentSettings");
            }
            ImGui::PopStyleVar();

            if (ImGui::BeginPopup("ComponentSettings"))
            {
                if (ImGui::MenuItem("Remove Component"))
                {
                    mDeleteSelected = true;
                }

                ImGui::EndPopup();
            }

            if (mOpenPanel)
            {
                auto& color = entity.GetComponent<SpriteRendererComponent>().Color;
                ImGui::ColorEdit4("Color", glm::value_ptr(color));
                ImGui::TreePop();
            }

            if (mDeleteSelected)
            {
                entity.RemoveComponent<SpriteRendererComponent>();
                mDeleteSelected = false;
            }
        }
    }
}