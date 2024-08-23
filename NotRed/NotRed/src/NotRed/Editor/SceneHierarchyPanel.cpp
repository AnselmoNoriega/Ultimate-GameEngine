#include "nrpch.h"
#include "SceneHierarchyPanel.h"

#include <imgui.h>

#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Script/ScriptEngine.h"
#include <assimp/scene.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

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

    void SceneHierarchyPanel::SetSelected(Entity entity)
    {
        mSelectionContext = entity;
    }

    void SceneHierarchyPanel::ImGuiRender()
    {
        ImGui::Begin("Scene Hierarchy");

        uint32_t entityCount = 0, meshCount = 0;
        auto view = mContext->mRegistry.view<TagComponent>();
        view.each([&](entt::entity entity, auto& name)
            {
                DrawEntityNode(Entity(entity, mContext.Raw()));
            });

        if (ImGui::BeginPopupContextWindow(0, 1))
        {
            if (ImGui::MenuItem("Create Empty Entity"))
            {
                mContext->CreateEntity("Empty Entity");
            }
            ImGui::EndPopup();
        }

        ImGui::End();

        ImGui::Begin("Properties");

        if (mSelectionContext)
        {
            DrawComponents(mSelectionContext);
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

    void SceneHierarchyPanel::DrawEntityNode(Entity entity)
    {
        const char* name = "Entity";
        name = entity.GetComponent<TagComponent>().Tag.c_str();

        ImGuiTreeNodeFlags node_flags = (entity == mSelectionContext ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        bool opened = ImGui::TreeNodeEx((void*)(uint32_t)entity, node_flags, name);
        if (ImGui::IsItemClicked())
        {
            mSelectionContext = entity;
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
            if (entity.HasComponent<MeshComponent>())
            {
                auto mesh = entity.GetComponent<MeshComponent>().MeshObj;
            }

            ImGui::TreePop();
        }

        if (entityDeleted)
        {
            mContext->DestroyEntity(entity);
            if (entity == mSelectionContext)
            {
                mSelectionContext = {};
            }
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

    static bool Property(const char* label, std::string& value)
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
        if (ImGui::InputText(sIDBuffer, buffer, 256))
        {
            value = buffer;
            modified = true;
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

    static void EndPropertyGrid()
    {
        ImGui::Columns(1);
        PopID();
    }

    void SceneHierarchyPanel::DrawComponents(Entity entity)
    {
        ImGui::AlignTextToFramePadding();

        {
            auto& tag = entity.GetComponent<TagComponent>().Tag;
            char buffer[256];
            memset(buffer, 0, 256);
            memcpy(buffer, tag.c_str(), tag.length());
            if (ImGui::InputText("##Tag", buffer, 256))
            {
                tag = std::string(buffer);
            }
        }

        ImGui::Separator();

        {
            auto& tc = entity.GetComponent<TransformComponent>();
            if (ImGui::TreeNodeEx((void*)((uint32_t)entity | typeid(MeshComponent).hash_code()), ImGuiTreeNodeFlags_OpenOnArrow, "Transform"))
            {
                auto [translation, rotation, scale] = GetTransformDecomposition(tc);

                ImGui::Columns(2);
                ImGui::Text("Translation");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);

                if (ImGui::DragFloat3("##translation", glm::value_ptr(translation), 0.25f))
                {
                    tc.Transform[3] = glm::vec4(translation, 1.0f);
                }

                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Text("Scale");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);

                if (ImGui::DragFloat3("##scale", glm::value_ptr(scale), 0.25f))
                {

                }

                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Columns(1);

                // ImGui::Text("Translation: %.2f, %.2f, %.2f", translation.x, translation.y, translation.z);
                // ImGui::Text("Scale: %.2f, %.2f, %.2f", scale.x, scale.y, scale.z);
                ImGui::TreePop();
            }
        }

        ImGui::Separator();


        if (entity.HasComponent<MeshComponent>())
        {
            auto& mc = entity.GetComponent<MeshComponent>();
            if (ImGui::TreeNodeEx((void*)((uint32_t)entity | typeid(TransformComponent).hash_code()), ImGuiTreeNodeFlags_OpenOnArrow, "Mesh"))
            {
                ImGui::BeginDisabled(true);
                if (mc.MeshObj)
                {
                    ImGui::InputText("File Path", (char*)mc.MeshObj->GetFilePath().c_str(), 256);
                }
                else
                {
                    ImGui::InputText("File Path", (char*)"Null", 256);
                }
                ImGui::EndDisabled();
                ImGui::TreePop();
            }
            ImGui::Separator();
        }

        if (entity.HasComponent<CameraComponent>())
        {
            auto& cc = entity.GetComponent<CameraComponent>();
            if (ImGui::TreeNodeEx((void*)((uint32_t)entity | typeid(CameraComponent).hash_code()), ImGuiTreeNodeFlags_OpenOnArrow, "Camera"))
            {
                ImGui::TreePop();
            }
            ImGui::Separator();
        }

        if (entity.HasComponent<SpriteRendererComponent>())
        {
            auto& src = entity.GetComponent<SpriteRendererComponent>();
            if (ImGui::TreeNodeEx((void*)((uint32_t)entity | typeid(SpriteRendererComponent).hash_code()), ImGuiTreeNodeFlags_OpenOnArrow, "Sprite Renderer"))
            {

                ImGui::TreePop();
            }
            ImGui::Separator();
        }

        if (entity.HasComponent<ScriptComponent>())
        {
            auto& sc = entity.GetComponent<ScriptComponent>();
            if (ImGui::TreeNodeEx((void*)((uint32_t)entity | typeid(ScriptComponent).hash_code()), ImGuiTreeNodeFlags_OpenOnArrow, "Script"))
            {
                BeginPropertyGrid();
                Property("Module Name", sc.ModuleName.c_str());

                // Public Fields
                auto& fieldMap = ScriptEngine::GetFieldMap();
                if (fieldMap.find(sc.ModuleName) != fieldMap.end())
                {
                    auto& publicFields = fieldMap.at(sc.ModuleName);
                    for (auto& field : publicFields)
                    {
                        switch (field.Type)
                        {
                        case FieldType::Int:
                        {
                            int value = field.GetValue<int>();
                            if (Property(field.Name.c_str(), value))
                            {
                                field.SetValue(value);
                            }
                            break;
                        }
                        case FieldType::Float:
                        {
                            float value = field.GetValue<float>();
                            if (Property(field.Name.c_str(), value, 0.2f))
                            {
                                field.SetValue(value);
                            }
                            break;
                        }
                        case FieldType::Vec2:
                        {
                            glm::vec2 value = field.GetValue<glm::vec2>();
                            if (Property(field.Name.c_str(), value, 0.2f))
                            {
                                field.SetValue(value);
                            }
                            break;
                        }
                        }
                    }
                }

                EndPropertyGrid();
                if (ImGui::Button("Run Script"))
                {
                    ScriptEngine::CreateEntity(entity);
                }
                ImGui::TreePop();
            }
            ImGui::Separator();
        }

    }
}