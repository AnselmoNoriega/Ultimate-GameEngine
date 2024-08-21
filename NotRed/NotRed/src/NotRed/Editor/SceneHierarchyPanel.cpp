#include "nrpch.h"
#include "SceneHierarchyPanel.h"

#include <imgui.h>

#include "NotRed/Renderer/Mesh.h"
#include <assimp/scene.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

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
    }

    void SceneHierarchyPanel::ImGuiRender()
    {
        ImGui::Begin("Scene Hierarchy");

        uint32_t entityCount = 0, meshCount = 0;
        auto& sceneEntities = mContext->mEntities;
        for (Entity* entity : sceneEntities)
        {
            DrawEntityNode(entity, entityCount, meshCount);
        }

        ImGui::End();

        ImGui::Begin("Properties");

        if (mSelectionContext)
        {
            /*auto mesh = mSelectionContext;
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
            }*/
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

    void SceneHierarchyPanel::DrawEntityNode(Entity* entity, uint32_t& imguiEntityID, uint32_t& imguiMeshID)
    {
        const char* name = entity->GetName().c_str();
        static char imguiName[128];
        memset(imguiName, 0, 128);
        sprintf(imguiName, "%s##%d", name, imguiEntityID++);
        if (ImGui::TreeNode(imguiName))
        {
            auto mesh = entity->GetMesh();
            auto material = entity->GetMaterial();
            const auto& transform = entity->GetTransform();

            if (mesh)
            {
                DrawMeshNode(mesh, imguiMeshID);
            }

            ImGui::TreePop();
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
        for (uint32_t i = 0; i < node->mNumMeshes; i++)
        {
            uint32_t meshIndex = node->mMeshes[i];
            mesh->mSubmeshes[meshIndex].Transform = transform;
        }

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
}