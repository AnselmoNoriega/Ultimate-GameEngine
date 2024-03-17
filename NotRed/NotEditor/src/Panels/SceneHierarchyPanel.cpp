#include "SceneHierarchyPanel.h"

#include <imgui/imgui.h>
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

        ImGui::End();
        ImGui::Begin("Properties");

        if (mSelectionContext)
        {
            DrawComponents(mSelectionContext);
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

        if (isOpened)
        {
            ImGui::TreePop();
        }
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

        if (ImGui::TreeNodeEx((void*)typeid(TransformComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Transform"))
        {
            auto& transform = entity.GetComponent<TransformComponent>().Transform;
            ImGui::DragFloat3("Position", glm::value_ptr(transform[3]), 0.2f);

            ImGui::TreePop();
        }

        if (entity.HasComponent<CameraComponent>())
        {
            if (ImGui::TreeNodeEx((void*)typeid(CameraComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Camera"))
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
        }

        if (entity.HasComponent<SpriteRendererComponent>())
        {
            if (ImGui::TreeNodeEx((void*)typeid(SpriteRendererComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Sprite Renderer"))
            {
                auto& color = entity.GetComponent<SpriteRendererComponent>().Color;
                ImGui::ColorEdit4("Color", glm::value_ptr(color));

                ImGui::TreePop();
            }
        }
    }
}