#include "nrpch.h"
#include "SceneHierarchyPanel.h"

#include <imgui.h>
#include <imgui/imgui_internal.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "NotRed/Core/Application.h"
#include "NotRed/Scene/Prefab.h"
#include "NotRed/Math/Math.h"
#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Script/ScriptEngine.h"
#include "NotRed/Physics/PhysicsManager.h"
#include "NotRed/Physics/PhysicsActor.h"
#include "NotRed/Physics/PhysicsLayer.h"
#include "NotRed/Physics/CookingFactory.h"
#include "NotRed/Renderer/MeshFactory.h"

#include "NotRed/Asset/AssetManager.h"

#include "NotRed/ImGui/ImGui.h"
#include "NotRed/ImGui/CustomTreeNode.h"
#include "NotRed/Renderer/Renderer.h"

#include "NotRed/Audio/AudioEngine.h"
#include "NotRed/Audio/AudioComponent.h"

namespace NR
{
    static Ref<Texture2D> mGearIcon;

    SceneHierarchyPanel::SceneHierarchyPanel()
    {
        mPencilIcon = Texture2D::Create("Resources/Editor/pencil_icon.png");
        mPlusIcon = Texture2D::Create("Resources/Editor/plus_icon.png");
        mGearIcon = Texture2D::Create("Resources/Editor/gear_icon.png");
    }

    SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
        : mContext(context)
    {
        mPencilIcon = Texture2D::Create("Resources/Editor/pencil_icon.png");
        mPlusIcon = Texture2D::Create("Resources/Editor/plus_icon.png");
        mGearIcon = Texture2D::Create("Resources/Editor/gear_icon.png");
    }

    SceneHierarchyPanel::~SceneHierarchyPanel()
    {
        mGearIcon.~Ref();
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

        if (mSelectionChangedCallback)
        {
            mSelectionChangedCallback(mSelectionContext);
        }
    }

    void SceneHierarchyPanel::ImGuiRender(bool window)
    {
        if (window)
        {
            UI::ScopedStyle padding(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("Scene Hierarchy");
        }

        ImRect windowRect = { ImGui::GetWindowContentRegionMin(), ImGui::GetWindowContentRegionMax() };

        if (mContext)
        {
            {
                const float edgeOffset = 4.0f;
                UI::ShiftCursorX(edgeOffset * 3.0f);
                UI::ShiftCursorY(edgeOffset * 2.0f);

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - edgeOffset * 3.0f);
                static std::string searchedString;
                UI::SearchWidget(searchedString);
                ImGui::Spacing();
                ImGui::Spacing();

                // Entity list
                //------------
                UI::ScopedStyle cellPadding(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 0.0f));

                // Alt row color
                const ImU32 colRowAlt = UI::ColorWithMultipliedValue(Colors::Theme::backgroundDark, 1.3f);
                UI::ScopedColor tableBGAlt(ImGuiCol_TableRowBgAlt, colRowAlt);

                // Table
                {
                    // Scrollable Table uses child window internally
                    UI::ScopedColor tableBg(ImGuiCol_ChildBg, Colors::Theme::backgroundDark);
                    ImGuiTableFlags tableFlags = ImGuiTableFlags_NoPadInnerX
                        | ImGuiTableFlags_Resizable
                        | ImGuiTableFlags_Reorderable
                        | ImGuiTableFlags_ScrollY;

                    const int numColumns = 3;
                    if (ImGui::BeginTable(UI::GenerateID(), numColumns, tableFlags, ImVec2(ImGui::GetContentRegionAvail())))
                    {
                        ImGui::TableSetupColumn("Label");
                        ImGui::TableSetupColumn("Type");
                        ImGui::TableSetupColumn("Visibility");

                        // Headers
                        {
                            const ImU32 colActive = UI::ColorWithMultipliedValue(Colors::Theme::groupHeader, 1.2f);
                            UI::ScopedColorStack headerColors(ImGuiCol_HeaderHovered, colActive,
                                ImGuiCol_HeaderActive, colActive);

                            ImGui::TableSetupScrollFreeze(ImGui::TableGetColumnCount(), 1);
                            ImGui::TableNextRow(ImGuiTableRowFlags_Headers, 22.0f);

                            for (int column = 0; column < ImGui::TableGetColumnCount(); column++)
                            {
                                ImGui::TableSetColumnIndex(column);
                                const char* column_name = ImGui::TableGetColumnName(column);
                                UI::ScopedID columnID(column);

                                UI::ShiftCursor(edgeOffset * 3.0f, edgeOffset * 2.0f);
                                ImGui::TableHeader(column_name);
                                UI::ShiftCursor(-edgeOffset * 3.0f, -edgeOffset * 2.0f);
                            }

                            ImGui::SetCursorPosX(ImGui::GetCurrentTable()->OuterRect.Min.x);
                            UI::Draw::Underline(true, 0.0f, 5.0f);
                        }

                        // List
                        {
                            // We draw selection and hover for table rows manually
                            UI::ScopedColorStack entitySelection(ImGuiCol_Header, IM_COL32_DISABLE,
                                ImGuiCol_HeaderHovered, IM_COL32_DISABLE,
                                ImGuiCol_HeaderActive, IM_COL32_DISABLE);

                            for (auto entity : mContext->mRegistry.view<IDComponent, RelationshipComponent>())
                            {
                                Entity e(entity, mContext.Raw());
                                if (e.GetParentID() == 0)
                                {
                                    DrawEntityNode({ entity, mContext.Raw() }, searchedString);
                                }
                            }
                        }

                        if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
                        {
                            if (ImGui::BeginMenu("Create"))
                            {
                                if (ImGui::MenuItem("Empty Entity"))
                                {
                                    auto newEntity = mContext->CreateEntity("Entity");
                                    SetSelected(newEntity);
                                }
                                if (ImGui::MenuItem("Camera"))
                                {
                                    auto newEntity = mContext->CreateEntity("Camera");
                                    newEntity.AddComponent<CameraComponent>();
                                    SetSelected(newEntity);
                                }
                                if (ImGui::MenuItem("Text"))
                                {
                                    auto newEntity = mContext->CreateEntity("Text");
                                    newEntity.AddComponent<TextComponent>();
                                    SetSelected(newEntity);
                                }
                                if (ImGui::BeginMenu("3D"))
                                {
                                    if (ImGui::MenuItem("Cube"))
                                    {
                                        auto newEntity = mContext->CreateEntity("Cube");
                                        Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>("Meshes/Default/Cube.nrmesh");
                                        newEntity.AddComponent<MeshComponent>(mesh);
                                        auto& bcc = newEntity.AddComponent<BoxColliderComponent>();
                                        bcc.DebugMesh = MeshFactory::CreateBox(bcc.Size);
                                        SetSelected(newEntity);
                                    }
                                    if (ImGui::MenuItem("Sphere"))
                                    {
                                        auto newEntity = mContext->CreateEntity("Sphere");
                                        Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>("Meshes/Default/Sphere.nrmesh");
                                        newEntity.AddComponent<MeshComponent>(mesh);
                                        auto& scc = newEntity.AddComponent<SphereColliderComponent>();
                                        scc.DebugMesh = MeshFactory::CreateSphere(scc.Radius);
                                        SetSelected(newEntity);
                                    }
                                    if (ImGui::MenuItem("Capsule"))
                                    {
                                        auto newEntity = mContext->CreateEntity("Capsule");
                                        Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>("Meshes/Default/Capsule.nrmesh");
                                        newEntity.AddComponent<MeshComponent>(mesh);
                                        CapsuleColliderComponent& ccc = newEntity.AddComponent<CapsuleColliderComponent>();
                                        ccc.DebugMesh = MeshFactory::CreateCapsule(ccc.Radius, ccc.Height);
                                        SetSelected(newEntity);
                                    }
                                    if (ImGui::MenuItem("Cylinder"))
                                    {
                                        auto newEntity = mContext->CreateEntity("Cylinder");
                                        Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>("Meshes/Default/Cylinder.nrmesh");
                                        newEntity.AddComponent<MeshComponent>(mesh);
                                        auto& collider = newEntity.AddComponent<MeshColliderComponent>(mesh);
                                        CookingFactory::CookMesh(collider);
                                        SetSelected(newEntity);
                                    }
                                    if (ImGui::MenuItem("Torus"))
                                    {
                                        auto newEntity = mContext->CreateEntity("Torus");
                                        Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>("Meshes/Default/Torus.nrmesh");
                                        newEntity.AddComponent<MeshComponent>(mesh);
                                        auto& collider = newEntity.AddComponent<MeshColliderComponent>(mesh);
                                        CookingFactory::CookMesh(collider);
                                        SetSelected(newEntity);
                                    }
                                    if (ImGui::MenuItem("Plane"))
                                    {
                                        auto newEntity = mContext->CreateEntity("Plane");
                                        Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>("Meshes/Default/Plane.nrmesh");
                                        newEntity.AddComponent<MeshComponent>(mesh);
                                        auto& collider = newEntity.AddComponent<MeshColliderComponent>(mesh);
                                        CookingFactory::CookMesh(collider);
                                        SetSelected(newEntity);
                                    }
                                    if (ImGui::MenuItem("Cone"))
                                    {
                                        auto newEntity = mContext->CreateEntity("Cone");
                                        Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>("Meshes/Default/Cone.fbx");
                                        newEntity.AddComponent<MeshComponent>(mesh);
                                        auto& collider = newEntity.AddComponent<MeshColliderComponent>(mesh);
                                        CookingFactory::CookMesh(collider);
                                        SetSelected(newEntity);
                                    }
                                    ImGui::EndMenu();
                                }
                                ImGui::Separator();
                                if (ImGui::MenuItem("Directional Light"))
                                {
                                    auto newEntity = mContext->CreateEntity("Directional Light");
                                    newEntity.AddComponent<DirectionalLightComponent>();
                                    newEntity.GetComponent<TransformComponent>().Rotation = glm::radians(glm::vec3{ 80.0f, 10.0f, 0.0f });
                                    SetSelected(newEntity);
                                }
                                if (ImGui::MenuItem("Point Light"))
                                {
                                    auto newEntity = mContext->CreateEntity("Point Light");
                                    newEntity.AddComponent<PointLightComponent>();
                                    newEntity.GetComponent<TransformComponent>().Translation = glm::vec3{ 0 };
                                    SetSelected(newEntity);
                                }
                                if (ImGui::MenuItem("Sky Light"))
                                {
                                    auto newEntity = mContext->CreateEntity("Sky Light");
                                    newEntity.AddComponent<SkyLightComponent>();
                                    SetSelected(newEntity);
                                }
                                if (ImGui::MenuItem("Particles"))
                                {
                                    auto newEntity = mContext->CreateEntity("Galxy Particles");
                                    newEntity.AddComponent<ParticleComponent>();
                                    SetSelected(newEntity);
                                }
                                ImGui::EndMenu();
                            }
                            ImGui::EndPopup();
                        }

                        ImGui::EndTable();
                    }
                }
            }

            if (ImGui::BeginDragDropTargetCustom(windowRect, ImGui::GetCurrentWindow()->ID))
            {
                const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("scene_entity_hierarchy", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);

                if (payload)
                {
                    Entity& entity = *(Entity*)payload->Data;
                    mContext->UnparentEntity(entity);
                }

                ImGui::EndDragDropTarget();
            }

            ImGui::End();

            {
                UI::ScopedStyle windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(2.0, 4.0f));
                ImGui::Begin("Properties");
            }

            if (mSelectionContext)
            {
                DrawComponents(mSelectionContext);
            }
        }

        if (window)
        {
            ImGui::End();
        }
    }

    void SceneHierarchyPanel::DrawEntityNode(Entity entity, const std::string& searchFilter)
    {
        const char* name = "Entity";
        if (entity.HasComponent<TagComponent>())
        {
            name = entity.GetComponent<TagComponent>().Tag.c_str();
        }

        const bool hasChildMatchingSearch = [&]
            {
                if (searchFilter.empty())
                {
                    return false;
                }

                for (auto child : entity.Children())
                {
                    Entity e = mContext->FindEntityByID(child);
                    if (e.HasComponent<TagComponent>())
                    {
                        if (UI::IsMatchingSearch(e.GetComponent<TagComponent>().Tag, searchFilter))
                        {
                            return true;
                        }
                    }
                }
                return false;
            }();

        if (!UI::IsMatchingSearch(name, searchFilter) && !hasChildMatchingSearch)
        {
            return;
        }

        const float edgeOffset = 4.0f;
        const float rowHeight = 21.0f;

        // ImGui item height tweaks
        auto* window = ImGui::GetCurrentWindow();
        window->DC.CurrLineSize.y = rowHeight;

        //---------------------------------------------

        ImGui::TableNextRow(0, rowHeight);

        // Label column
        //-------------
        ImGui::TableNextColumn();
        window->DC.CurrLineTextBaseOffset = 3.0f;
        const ImVec2 rowAreaMin = ImGui::TableGetCellBgRect(ImGui::GetCurrentTable(), 0).Min;
        const ImVec2 rowAreaMax = { ImGui::TableGetCellBgRect(ImGui::GetCurrentTable(), ImGui::TableGetColumnCount() - 1).Max.x,
                                    rowAreaMin.y + rowHeight };

        const bool isSelected = entity == mSelectionContext;
        ImGuiTreeNodeFlags flags = (isSelected ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

        if (hasChildMatchingSearch)
        {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }
        if (entity.Children().empty())
        {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        ImGui::PushClipRect(rowAreaMin, rowAreaMax, false);
        const bool isRowHovered = [&rowAreaMin, &rowAreaMax, &name]
            {
                return ImGui::ItemHoverable(ImRect(rowAreaMin, rowAreaMax), ImGui::GetID(name));
            }();

        ImGui::PopClipRect();

        const bool isWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        // Row coloring
        //--------------
        // Fill with light selection color if any of the child entities selected
        auto isAnyDescendantSelected = [&](Entity ent, auto isAnyDescendantSelected) -> bool
            {
                if (ent == mSelectionContext)
                {
                    return true;
                }

                if (!ent.Children().empty())
                {
                    for (auto& childEntityID : ent.Children())
                    {
                        Entity childEntity = mContext->FindEntityByID(childEntityID);
                        if (isAnyDescendantSelected(childEntity, isAnyDescendantSelected))
                        {
                            return true;
                        }
                    }
                }

                return false;
            };
        auto fillRowWithColor = [](const ImColor& color)
            {
                for (int column = 0; column < ImGui::TableGetColumnCount(); column++)
                {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, color, column);
                }
            };

        if (isSelected)
        {
            if (isWindowFocused || UI::NavigatedTo())
            {
                fillRowWithColor(Colors::Theme::selection);
            }
            else
            {
                const ImColor col = UI::ColorWithMultipliedValue(Colors::Theme::selection, 0.9f);
                fillRowWithColor(UI::ColorWithMultipliedSaturation(col, 0.7f));
            }
        }
        else if (isRowHovered)
        {
            fillRowWithColor(Colors::Theme::groupHeader);
        }
        else if (isAnyDescendantSelected(entity, isAnyDescendantSelected))
        {
            fillRowWithColor(Colors::Theme::selectionMuted);
        }

        // Text coloring
        //---------------
        if (isSelected)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::Theme::backgroundDark);
        }

        const bool missingMesh = entity.HasComponent<MeshComponent>() &&
            (
                entity.GetComponent<MeshComponent>().MeshObj &&
                entity.GetComponent<MeshComponent>().MeshObj->IsFlagSet(AssetFlag::Missing)
                );

        if (missingMesh)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.4f, 0.3f, 1.0f));
        }

        bool isPrefab = entity.HasComponent<PrefabComponent>();
        if (isPrefab && !isSelected)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.32f, 0.7f, 0.87f, 1.0f));
        }

        // Tree node
        //----------
        const bool opened = ImGui::TreeNodeWithIcon(nullptr, (void*)(uint64_t)(uint32_t)entity, flags, IM_COL32_WHITE, name);

        bool entityDeleted = false;
        if (ImGui::BeginPopupContextItem())
        {
            {
                UI::ScopedColor colText(ImGuiCol_Text, Colors::Theme::text);
                UI::ScopedColorStack entitySelection(ImGuiCol_Header, Colors::Theme::groupHeader,
                    ImGuiCol_HeaderHovered, Colors::Theme::groupHeader,
                    ImGuiCol_HeaderActive, Colors::Theme::groupHeader);

                if (isPrefab)
                {
                    if (ImGui::MenuItem("Update Prefab"))
                    {
                        AssetHandle prefabAssetHandle = entity.GetComponent<PrefabComponent>().PrefabID;
                        Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(prefabAssetHandle);
                        if (prefab)
                        {
                            prefab->Create(entity);
                        }
                        else
                        {
                            NR_ERROR("Prefab has invalid asset handle: {0}", prefabAssetHandle);
                        }
                    }
                }

                if (ImGui::BeginMenu("Create"))
                {
                    if (ImGui::MenuItem("Empty Entity"))
                    {
                        auto newEntity = mContext->CreateEntity("Empty Entity");
                        mContext->ParentEntity(newEntity, entity);
                        SetSelected(newEntity);
                    }
                    if (ImGui::MenuItem("Camera"))
                    {
                        auto newEntity = mContext->CreateEntity("Camera");
                        mContext->ParentEntity(newEntity, entity);
                        newEntity.AddComponent<CameraComponent>();
                        SetSelected(newEntity);
                    }/*
                    if (ImGui::MenuItem("Text"))
                    {
                        auto newEntity = mContext->CreateEntity("Text");
                        mContext->ParentEntity(newEntity, entity);
                        newEntity.AddComponent<TextComponent>();
                        SetSelected(newEntity);
                    }*/
                    if (ImGui::BeginMenu("3D"))
                    {/*TODO
                        if (ImGui::MenuItem("Cube"))
                        {
                            auto newEntity = mContext->CreateEntity("Cube");
                            mContext->ParentEntity(newEntity, entity);
                            auto mesh = AssetManager::GetAssetHandleFromFilePath("Meshes/Default/Cube.nrmesh");
                            newEntity.AddComponent<MeshComponent>(mesh);
                            auto& bcc = newEntity.AddComponent<BoxColliderComponent>();
                            bcc.DebugMesh = MeshFactory::CreateBox(bcc.Size);
                            SetSelected(newEntity);
                        }
                        if (ImGui::MenuItem("Sphere"))
                        {
                            auto newEntity = mContext->CreateEntity("Sphere");
                            mContext->ParentEntity(newEntity, entity);
                            auto mesh = AssetManager::GetAssetHandleFromFilePath("Meshes/Default/Sphere.nrmesh");
                            newEntity.AddComponent<MeshComponent>(mesh);
                            auto& scc = newEntity.AddComponent<SphereColliderComponent>();
                            scc.DebugMesh = MeshFactory::CreateSphere(scc.Radius);
                            SetSelected(newEntity);
                        }
                        if (ImGui::MenuItem("Capsule"))
                        {
                            auto newEntity = mContext->CreateEntity("Capsule");
                            mContext->ParentEntity(newEntity, entity);
                            auto mesh = AssetManager::GetAssetHandleFromFilePath("Meshes/Default/Capsule.nrmesh");
                            newEntity.AddComponent<MeshComponent>(mesh);
                            CapsuleColliderComponent& ccc = newEntity.AddComponent<CapsuleColliderComponent>();
                            ccc.DebugMesh = MeshFactory::CreateCapsule(ccc.Radius, ccc.Height);
                            SetSelected(newEntity);
                        }
                        if (ImGui::MenuItem("Cylinder"))
                        {
                            auto newEntity = mContext->CreateEntity("Cylinder");
                            mContext->ParentEntity(newEntity, entity);
                            auto mesh = AssetManager::GetAssetHandleFromFilePath("Meshes/Default/Cylinder.nrmesh");
                            newEntity.AddComponent<MeshComponent>(mesh);
                            auto& collider = newEntity.AddComponent<MeshColliderComponent>(mesh);
                            CookingFactory::CookMesh(collider);
                            SetSelected(newEntity);
                        }
                        if (ImGui::MenuItem("Torus"))
                        {
                            auto newEntity = mContext->CreateEntity("Torus");
                            mContext->ParentEntity(newEntity, entity);
                            auto mesh = AssetManager::GetAssetHandleFromFilePath("Meshes/Default/Torus.nrmesh");
                            newEntity.AddComponent<MeshComponent>(mesh);
                            auto& collider = newEntity.AddComponent<MeshColliderComponent>(mesh);
                            CookingFactory::CookMesh(collider);
                            SetSelected(newEntity);
                        }
                        if (ImGui::MenuItem("Plane"))
                        {
                            auto newEntity = mContext->CreateEntity("Plane");
                            mContext->ParentEntity(newEntity, entity);
                            auto mesh = AssetManager::GetAssetHandleFromFilePath("Meshes/Default/Plane.nrmesh");
                            newEntity.AddComponent<MeshComponent>(mesh);
                            auto& collider = newEntity.AddComponent<MeshColliderComponent>(mesh);
                            CookingFactory::CookMesh(collider);
                            SetSelected(newEntity);
                        }
                        if (ImGui::MenuItem("Cone"))
                        {
                            auto newEntity = mContext->CreateEntity("Cone");
                            mContext->ParentEntity(newEntity, entity);
                            auto mesh = AssetManager::GetAssetHandleFromFilePath("Meshes/Default/Cone.fbx");
                            newEntity.AddComponent<MeshComponent>(mesh);
                            auto& collider = newEntity.AddComponent<MeshColliderComponent>(mesh);
                            CookingFactory::CookMesh(collider);
                            SetSelected(newEntity);
                        }*/
                        ImGui::EndMenu();
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Directional Light"))
                    {
                        auto newEntity = mContext->CreateEntity("Directional Light");
                        mContext->ParentEntity(newEntity, entity);
                        newEntity.AddComponent<DirectionalLightComponent>();
                        newEntity.GetComponent<TransformComponent>().Rotation = glm::radians(glm::vec3{ 80.0f, 10.0f, 0.0f });
                        SetSelected(newEntity);
                    }
                    if (ImGui::MenuItem("Point Light"))
                    {
                        auto newEntity = mContext->CreateEntity("Point Light");
                        mContext->ParentEntity(newEntity, entity);
                        newEntity.AddComponent<PointLightComponent>();
                        newEntity.GetComponent<TransformComponent>().Translation = glm::vec3{ 0 };
                        SetSelected(newEntity);
                    }
                    if (ImGui::MenuItem("Sky Light"))
                    {
                        auto newEntity = mContext->CreateEntity("Sky Light");
                        mContext->ParentEntity(newEntity, entity);
                        newEntity.AddComponent<SkyLightComponent>();
                        SetSelected(newEntity);
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem("Delete"))
                {
                    entityDeleted = true;
                }
            }

            ImGui::EndPopup();
        }

        // Type column
        //------------
        ImGui::PushClipRect(rowAreaMin, rowAreaMax, false);
        const bool isRowClicked = [&rowAreaMin, &rowAreaMax, isRowHovered]
            {
                if (!isRowHovered)
                {
                    return false;
                }

                auto* table = ImGui::GetCurrentTable();
                bool hoveringResizer = false;
                for (int column = 0; column < ImGui::TableGetColumnCount(); column++)
                {
                    const auto resizerID = ImGui::TableGetColumnResizeID(table, column);
                    if (ImGui::GetHoveredID() == resizerID)
                        hoveringResizer = true;
                }

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !hoveringResizer)
                {
                    return true;
                }

                return false;
            }();

        ImGui::PopClipRect();

        if (isRowClicked)
        {
            mSelectionContext = entity;
            if (mSelectionChangedCallback)
            {
                mSelectionChangedCallback(mSelectionContext);
            }

            ImGui::FocusWindow(ImGui::GetCurrentWindow());
        }

        if (missingMesh)
        {
            ImGui::PopStyleColor();
        }

        if (isSelected)
        {
            ImGui::PopStyleColor();
        }

        // Drag & Drop
        //------------
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            ImGui::Text(entity.GetComponent<TagComponent>().Tag.c_str());
            ImGui::SetDragDropPayload("scene_entity_hierarchy", &entity, sizeof(Entity));
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget())
        {
            const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("scene_entity_hierarchy", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);

            if (payload)
            {
                Entity& droppedEntity = *(Entity*)payload->Data;
                mContext->ParentEntity(droppedEntity, entity);
            }

            ImGui::EndDragDropTarget();
        }

        ImGui::TableNextColumn();
        if (isPrefab)
        {
            UI::ShiftCursorX(edgeOffset * 3.0f);
            if (isSelected)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, Colors::Theme::backgroundDark);
            }

            ImGui::TextUnformatted("Prefab");
            ImGui::PopStyleColor();
        }

        // Draw children
        //--------------
        if (opened)
        {
            for (auto child : entity.Children())
            {
                Entity e = mContext->FindEntityByID(child);
                if (e)
                {
                    DrawEntityNode(e, searchFilter);
                }
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
            mEntityDeletedCallback(entity);
        }
    }

    template<typename T, typename UIFunction>
    static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction, bool canBeRemoved = true)
    {
        if (entity.HasComponent<T>())
        {
            ImGui::PushID((void*)typeid(T).hash_code());
            auto& component = entity.GetComponent<T>();
            ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

            bool open = UI::PropertyGridHeader(name);
            bool right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
            float lineHeight = ImGui::GetItemRectMax().y - ImGui::GetItemRectMin().y;

            bool resetValues = false;
            bool removeComponent = false;

            ImGui::SameLine(contentRegionAvailable.x - lineHeight);
            if (ImGui::InvisibleButton("##options", ImVec2{ lineHeight, lineHeight }) || right_clicked)
            {
                ImGui::OpenPopup("ComponentSettings");
            }

            UI::DrawButtonImage(mGearIcon, IM_COL32(160, 160, 160, 200),
                IM_COL32(160, 160, 160, 255),
                IM_COL32(160, 160, 160, 150),
                UI::RectExpanded(UI::GetItemRect(), -6.0f, -6.0f));

            if (UI::BeginPopup("ComponentSettings"))
            {
                if (ImGui::MenuItem("Reset"))
                {
                    resetValues = true;
                }

                if (canBeRemoved)
                {
                    if (ImGui::MenuItem("Remove component"))
                    {
                        removeComponent = true;
                    }
                }

                UI::EndPopup();
            }

            if (open)
            {
                uiFunction(component);
                ImGui::TreePop();
            }

            if (removeComponent || resetValues)
            {
                entity.RemoveComponent<T>();
            }

            if (resetValues)
            {
                entity.AddComponent<T>();
            }

            if (!open)
            {
                UI::ShiftCursorY(-(ImGui::GetStyle().ItemSpacing.y + 1.0f));
            }

            ImGui::PopID();
        }
    }

    static bool DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
    {
        bool modified = false;

        UI::PushID();
        ImGui::TableSetColumnIndex(0);
        UI::ShiftCursor(17.0f, 7.0f);
        ImGui::Text(label.c_str());
        UI::Draw::Underline(false, 0.0f, 2.0f);

        ImGui::TableSetColumnIndex(1);
        UI::ShiftCursor(7.0f, 0.0f);
        {
            const float spacingX = 8.0f;
            UI::ScopedStyle itemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2{ spacingX, 0.0f });
            UI::ScopedStyle padding(ImGuiStyleVar_WindowPadding, ImVec2{ 0.0f, 2.0f });

            {
                // Begin XYZ area
                UI::ScopedColor padding(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
                UI::ScopedStyle frame(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
                ImGui::BeginChild(ImGui::GetID((label + "fr").c_str()),
                    ImVec2(ImGui::GetContentRegionAvail().x - spacingX, ImGui::GetFrameHeightWithSpacing() + 8.0f),
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
                );
            }

            constexpr float framePadding = 2.0f;
            constexpr float outlineSpacing = 1.0f;
            const float lineHeight = GImGui->Font->FontSize + framePadding * 2.0f;

            const ImVec2 buttonSize = { lineHeight + 2.0f, lineHeight };

            const float inputItemWidth = (ImGui::GetContentRegionAvail().x - spacingX) / 3.0f - buttonSize.x;

            const ImGuiIO& io = ImGui::GetIO();
            auto boldFont = io.Fonts->Fonts[0];

            auto drawControl = [&](const std::string& label, float& value, const ImVec4& colorN, const ImVec4& colorH, const ImVec4& colorP)
                {
                    {
                        UI::ScopedStyle buttonFrame(ImGuiStyleVar_FramePadding, ImVec2(framePadding, 0.0f));
                        UI::ScopedStyle buttonRounding(ImGuiStyleVar_FrameRounding, 1.0f);
                        UI::ScopedColorStack buttonColors(ImGuiCol_Button, colorN,
                            ImGuiCol_ButtonHovered, colorH,
                            ImGuiCol_ButtonActive, colorP);

                        UI::ScopedFont buttonFont(boldFont);

                        UI::ShiftCursorY(2.0f);
                        if (ImGui::Button(label.c_str(), buttonSize))
                        {
                            value = resetValue;
                            modified = true;
                        }
                    }

                    ImGui::SameLine(0.0f, outlineSpacing);
                    ImGui::SetNextItemWidth(inputItemWidth);
                    UI::ShiftCursorY(-2.0f);
                    modified |= ImGui::DragFloat(("##" + label).c_str(), &value, 0.1f, 0.0f, 0.0f, "%.2f");

                    if (!UI::IsItemDisabled())
                    {
                        UI::DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
                    }
                };

            drawControl("X", values.x, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f }, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f }, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
            ImGui::SameLine(0.0f, outlineSpacing);
            drawControl("Y", values.y, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f }, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f }, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
            ImGui::SameLine(0.0f, outlineSpacing);
            drawControl("Z", values.z, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f }, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f }, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
            ImGui::EndChild();
        }

        UI::PopID();

        return modified;
    }

    void SceneHierarchyPanel::DrawComponents(Entity entity)
    {
        ImGui::AlignTextToFramePadding();

        auto id = entity.GetComponent<IDComponent>().ID;

        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

        UI::ShiftCursor(4.0f, 4.0f);
        bool isHoveringID = false;

        if (entity.HasComponent<TagComponent>())
        {
            const float iconOffset = 6.0f;
            UI::ShiftCursor(4.0f, iconOffset);
            UI::Image(mPencilIcon, ImVec2(mPencilIcon->GetWidth(), mPencilIcon->GetHeight()),
                ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
                ImColor(128, 128, 128, 255).Value);
            ImGui::SameLine(0.0f, 4.0f);

            UI::ShiftCursorY(-iconOffset);

            auto& tag = entity.GetComponent<TagComponent>().Tag;
            char buffer[256];
            memset(buffer, 0, 256);
            memcpy(buffer, tag.c_str(), tag.length());
            ImGui::PushItemWidth(contentRegionAvailable.x * 0.5f);

            UI::ScopedStyle frameBorder(ImGuiStyleVar_FrameBorderSize, 0.0f);
            UI::ScopedColor frameColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
            UI::ScopedFont boldFont(ImGui::GetIO().Fonts->Fonts[0]);

            if (ImGui::InputText("##Tag", buffer, 256))
            {
                tag = std::string(buffer);
            }

            UI::DrawItemActivityOutline(2.0f, false, Colors::Theme::accent);

            isHoveringID = ImGui::IsItemHovered();
            ImGui::PopItemWidth();
        }

        // ID
        if (isHoveringID)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("%u", id);
        }

        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 textSize = ImGui::CalcTextSize(" ADD        ");
        textSize.x += GImGui->Style.FramePadding.x * 2.0f;
        {
            UI::ScopedColorStack addCompButtonColors(ImGuiCol_Button, IM_COL32(70, 70, 70, 200),
                ImGuiCol_ButtonHovered, IM_COL32(70, 70, 70, 255),
                ImGuiCol_ButtonActive, IM_COL32(70, 70, 70, 150));

            ImGui::SameLine(contentRegionAvailable.x - (textSize.x + GImGui->Style.FramePadding.x));

            if (ImGui::Button(" ADD       ", ImVec2(textSize.x + 4.0f, lineHeight + 2.0f)))
            {
                ImGui::OpenPopup("AddComponentPanel");
            }
            const float pad = 4.0f;
            const float iconWidth = ImGui::GetFrameHeight() - pad * 2.0f;
            const float iconHeight = iconWidth;

            ImVec2 iconPos = ImGui::GetItemRectMax();
            iconPos.x -= iconWidth + pad;
            iconPos.y -= iconHeight + pad;

            ImGui::SetCursorScreenPos(iconPos);
            UI::ShiftCursor(-5.0f, -1.0f);
            UI::Image(mPlusIcon, ImVec2(iconWidth, iconHeight),
                ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
                ImColor(160, 160, 160, 255).Value);
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        {
            UI::ScopedFont boldFont(ImGui::GetIO().Fonts->Fonts[0]);
            if (UI::BeginPopup("AddComponentPanel"))
            {
                if (!mSelectionContext.HasComponent<CameraComponent>())
                {
                    if (ImGui::MenuItem("Camera"))
                    {
                        mSelectionContext.AddComponent<CameraComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!mSelectionContext.HasComponent<MeshComponent>())
                {
                    if (ImGui::MenuItem("Mesh"))
                    {
                        mSelectionContext.AddComponent<MeshComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!mSelectionContext.HasComponent<AnimationComponent>())
                {
                    if (ImGui::Button("Animation"))
                    {
                        mSelectionContext.AddComponent<AnimationComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!mSelectionContext.HasComponent<DirectionalLightComponent>())
                {
                    if (ImGui::MenuItem("Directional Light"))
                    {
                        mSelectionContext.AddComponent<DirectionalLightComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!mSelectionContext.HasComponent<PointLightComponent>())
                {
                    if (ImGui::MenuItem("Point Light"))
                    {
                        mSelectionContext.AddComponent<PointLightComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!mSelectionContext.HasComponent<SkyLightComponent>())
                {
                    if (ImGui::MenuItem("Sky Light"))
                    {
                        mSelectionContext.AddComponent<SkyLightComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!mSelectionContext.HasComponent<ScriptComponent>())
                {
                    if (ImGui::MenuItem("Script"))
                    {
                        mSelectionContext.AddComponent<ScriptComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!mSelectionContext.HasComponent<SpriteRendererComponent>())
                {
                    if (ImGui::MenuItem("Sprite Renderer"))
                    {
                        mSelectionContext.AddComponent<SpriteRendererComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!mSelectionContext.HasComponent<TextComponent>())
                {
                    if (ImGui::Button("Text"))
                    {
                        mSelectionContext.AddComponent<TextComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!mSelectionContext.HasComponent<RigidBody2DComponent>())
                {
                    if (ImGui::MenuItem("Rigidbody 2D"))
                    {
                        mSelectionContext.AddComponent<RigidBody2DComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!mSelectionContext.HasComponent<BoxCollider2DComponent>())
                {
                    if (ImGui::MenuItem("Box Collider 2D"))
                    {
                        mSelectionContext.AddComponent<BoxCollider2DComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!mSelectionContext.HasComponent<CircleCollider2DComponent>())
                {
                    if (ImGui::MenuItem("Circle Collider 2D"))
                    {
                        mSelectionContext.AddComponent<CircleCollider2DComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!mSelectionContext.HasComponent<RigidBodyComponent>())
                {
                    if (ImGui::MenuItem("Rigidbody"))
                    {
                        mSelectionContext.AddComponent<RigidBodyComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!mSelectionContext.HasComponent<BoxColliderComponent>())
                {
                    if (ImGui::MenuItem("Box Collider"))
                    {
                        auto& bcc = mSelectionContext.AddComponent<BoxColliderComponent>();
                        bcc.DebugMesh = MeshFactory::CreateBox(bcc.Size);
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!mSelectionContext.HasComponent<SphereColliderComponent>())
                {
                    if (ImGui::MenuItem("Sphere Collider"))
                    {
                        auto& scc = mSelectionContext.AddComponent<SphereColliderComponent>();
                        scc.DebugMesh = MeshFactory::CreateSphere(scc.Radius);
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!mSelectionContext.HasComponent<CapsuleColliderComponent>())
                {
                    if (ImGui::MenuItem("Capsule Collider"))
                    {
                        auto& ccc = mSelectionContext.AddComponent<CapsuleColliderComponent>();
                        ccc.DebugMesh = MeshFactory::CreateCapsule(ccc.Radius, ccc.Height);
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!mSelectionContext.HasComponent<MeshColliderComponent>())
                {
                    if (ImGui::MenuItem("Mesh Collider"))
                    {
                        MeshColliderComponent& component = mSelectionContext.AddComponent<MeshColliderComponent>();
                        if (mSelectionContext.HasComponent<MeshComponent>())
                        {
                            component.CollisionMesh = mSelectionContext.GetComponent<MeshComponent>().MeshObj;
                            CookingFactory::CookMesh(component);
                        }

                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!mSelectionContext.HasComponent<AudioComponent>())
                {
                    if (ImGui::MenuItem("Audio"))
                    {
                        mSelectionContext.AddComponent<AudioComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!mSelectionContext.HasComponent<AudioListenerComponent>())
                {
                    if (ImGui::MenuItem("Audio Listener"))
                    {
                        auto view = mContext->GetAllEntitiesWith<AudioListenerComponent>();
                        bool listenerExists = !view.empty();
                        auto& listenerComponent = mSelectionContext.AddComponent<AudioListenerComponent>();
                        // If this is the only listener, make it Active (only affects Runtime)
                        listenerComponent.Active = !listenerExists;
                        AudioEngine::Get().RegisterNewListener(listenerComponent);
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!mSelectionContext.HasComponent<ParticleComponent>())
                {
                    if (ImGui::MenuItem("Particle"))
                    {
                        mSelectionContext.AddComponent<ParticleComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                UI::EndPopup();
            }
        }

        DrawComponent<TransformComponent>("Transform", entity, [](TransformComponent& component)
            {
                UI::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
                UI::ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));

                ImGui::BeginTable("transformComponent", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoClip);
                ImGui::TableSetupColumn("label_column", 0, 100.0f);
                ImGui::TableSetupColumn("value_column", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - 100.0f);
                ImGui::TableNextRow();

                DrawVec3Control("Translation", component.Translation);
                ImGui::TableNextRow();

                DrawVec3Control("Rotation", component.Rotation);
                ImGui::TableNextRow();

                DrawVec3Control("Scale", component.Scale, 1.0f);
                ImGui::EndTable();

                UI::ShiftCursorY(-8.0f);
                UI::Draw::Underline();
                UI::ShiftCursorY(18.0f);
            }, false);

        DrawComponent<MeshComponent>("Mesh", entity, [&](MeshComponent& mc)
            {
                UI::BeginPropertyGrid();

                UI::PropertyAssetReferenceError error;

                if (UI::PropertyAssetReferenceWithConversion<Mesh, MeshAsset>("Mesh", mc.MeshObj,
                    [=](Ref<MeshAsset> meshAsset)
                    {
                        if (mMeshAssetConvertCallback)
                        {
                            mMeshAssetConvertCallback(entity, meshAsset);
                        }
                    }, &error))
                {
                    if (entity.HasComponent<MeshColliderComponent>())
                    {
                        auto& mcc = entity.GetComponent<MeshColliderComponent>();
                        mcc.CollisionMesh = mc.MeshObj;
                        CookingFactory::CookMesh(mcc, true);
                    }
                }

                if (error == UI::PropertyAssetReferenceError::InvalidMetadata)
                {
                    if (mInvalidMetadataCallback)
                    {
                        mInvalidMetadataCallback(entity, UI::sPropertyAssetReferenceAssetHandle);
                    }
                }
                UI::EndPropertyGrid();

                if (mc.MeshObj && mc.MeshObj->IsValid())
                {
                    if (UI::BeginTreeNode("Materials"))
                    {
                        UI::BeginPropertyGrid();
                        auto meshMaterialTable = mc.MeshObj->GetMaterials();
                        if (mc.Materials->GetMaterialCount() < meshMaterialTable->GetMaterialCount())
                        {
                            mc.Materials->SetMaterialCount(meshMaterialTable->GetMaterialCount());
                        }

                        for (size_t i = 0; i < mc.Materials->GetMaterialCount(); ++i)
                        {
                            if (i == meshMaterialTable->GetMaterialCount())
                            {
                                ImGui::Separator();
                            }

                            bool hasLocalMaterial = mc.Materials->HasMaterial(i);
                            bool hasMeshMaterial = meshMaterialTable->HasMaterial(i);
                            Ref<MaterialAsset> meshMaterialAsset;
                            if (hasMeshMaterial)
                            {
                                meshMaterialAsset = meshMaterialTable->GetMaterial(i);
                            }

                            Ref<MaterialAsset> materialAsset = hasLocalMaterial ? mc.Materials->GetMaterial(i) : meshMaterialAsset;
                            std::string label = fmt::format("[Material {0}]", i);
                            UI::PropertyAssetReferenceSettings settings;
                            if (hasLocalMaterial || !hasMeshMaterial)
                            {
                                if (hasLocalMaterial)
                                {
                                    settings.AdvanceToNextColumn = false;
                                    settings.WidthOffset = ImGui::GetStyle().ItemSpacing.x + 28.0f;
                                }

                                UI::PropertyAssetReferenceTarget<MaterialAsset>(label.c_str(), nullptr, materialAsset, [i, materialTable = mc.Materials](Ref<MaterialAsset> materialAsset) mutable
                                    {
                                        materialTable->SetMaterial(i, materialAsset);
                                    }, settings);
                            }
                            else
                            {
                                std::string meshMaterialName = meshMaterialAsset->GetMaterial()->GetName();
                                if (meshMaterialName.empty())
                                {
                                    meshMaterialName = "Unnamed Material";
                                }

                                UI::PropertyAssetReferenceTarget<MaterialAsset>(label.c_str(), meshMaterialName.c_str(), materialAsset, [i, materialTable = mc.Materials](Ref<MaterialAsset> materialAsset) mutable
                                    {
                                        materialTable->SetMaterial(i, materialAsset);
                                    }, settings);
                            }

                            if (hasLocalMaterial)
                            {
                                ImGui::SameLine();
                                float prevItemHeight = ImGui::GetItemRectSize().y;
                                if (ImGui::Button("X", { prevItemHeight, prevItemHeight }))
                                {
                                    mc.Materials->ClearMaterial(i);
                                }
                                ImGui::NextColumn();
                            }
                        }
                        UI::EndPropertyGrid();
                        UI::EndTreeNode();
                    }
                }
            });

        DrawComponent<ParticleComponent>("Particles", entity, [&](ParticleComponent& pc)
            {
                UI::BeginPropertyGrid();

                if (UI::Property("Particle Count", pc.ParticleCount, 0, INT_MAX))
                {
                    if (pc.ParticleCount < 1)
                    {
                        pc.ParticleCount = 1;
                    }
                    pc.MeshObj = Ref<Mesh>::Create(Ref<MeshAsset>::Create(pc.ParticleCount));
                }

                if (UI::PropertyColor("Star Color", pc.StarColor))
                {
                    pc.MeshObj->GetMaterials()->GetMaterial(0)->GetMaterial()->Set("uGalaxySpecs.StarColor", pc.StarColor);
                }

                if (UI::PropertyColor("Dust Color", pc.DustColor))
                {
                    pc.MeshObj->GetMaterials()->GetMaterial(0)->GetMaterial()->Set("uGalaxySpecs.DustColor", pc.DustColor);
                }

                if (UI::PropertyColor("h2Region Color", pc.h2RegionColor))
                {
                    pc.MeshObj->GetMaterials()->GetMaterial(0)->GetMaterial()->Set("uGalaxySpecs.h2RegionColor", pc.h2RegionColor);
                }

                UI::EndPropertyGrid();
            });

        DrawComponent<AnimationComponent>("Animation", entity, [&](AnimationComponent& anim)
            {
                UI::BeginPropertyGrid();
                UI::PropertyAssetReferenceError error;

                UI::PropertyAssetReference<AnimationController>("Animation Controller", anim.AnimationController, &error);
                if (error == UI::PropertyAssetReferenceError::InvalidMetadata)
                {
                    if (mInvalidMetadataCallback)
                    {
                        mInvalidMetadataCallback(entity, UI::sPropertyAssetReferenceAssetHandle);
                    }
                }
                UI::EndPropertyGrid();

                if (entity.HasComponent<MeshComponent>())
                {
                    auto mc = entity.GetComponent<MeshComponent>();
                    if (!mc.MeshObj || !mc.MeshObj->IsValid() || !mc.MeshObj->IsRigged()) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
                        ImGui::Text("Mesh is not rigged for animation!");
                        ImGui::PopStyleColor();
                    }
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
                    ImGui::Text("Please add a Mesh component!");
                    ImGui::PopStyleColor();
                }
            });

        DrawComponent<CameraComponent>("Camera", entity, [](CameraComponent& cc)
            {
                UI::BeginPropertyGrid();

                // Projection Type
                const char* projTypeStrings[] = { "Perspective", "Orthographic" };
                int currentProj = (int)cc.CameraObj.GetProjectionType();
                if (UI::PropertyDropdown("Projection", projTypeStrings, 2, &currentProj))
                {
                    cc.CameraObj.SetProjectionType((SceneCamera::ProjectionType)currentProj);
                }

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

        DrawComponent<TextComponent>("Text", entity, [](TextComponent& tc)
            {
                UI::BeginPropertyGrid();
                std::string normalString(tc.TextString.begin(), tc.TextString.end());
                if (UI::Property("Text String", normalString))
                {
                    tc.TextString = std::u32string(normalString.begin(), normalString.end());
                }

                UI::PropertyAssetReferenceSettings settings;
                bool customFont = tc.FontAsset->Handle != Font::GetDefaultFont()->Handle;
                if (customFont)
                {
                    settings.AdvanceToNextColumn = false;
                    settings.WidthOffset = ImGui::GetStyle().ItemSpacing.x + 28.0f;
                }
                UI::PropertyAssetReference("Font", tc.FontAsset, nullptr, settings);
                if (customFont)
                {
                    ImGui::SameLine();
                    float prevItemHeight = ImGui::GetItemRectSize().y;
                    if (ImGui::Button("X", { prevItemHeight, prevItemHeight }))
                    {
                        tc.FontAsset = Font::GetDefaultFont();
                    }

                    ImGui::NextColumn();
                }

                UI::PropertyColor("Color", tc.Color);
                UI::Property("Line Spacing", tc.LineSpacing, 0.01f);
                UI::Property("Kerning", tc.Kerning, 0.01f);
                UI::Separator();
                UI::Property("Max Width", tc.MaxWidth);
                
                UI::EndPropertyGrid();
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

        DrawComponent<PointLightComponent>("Point Light", entity, [](PointLightComponent& dlc)
            {
                UI::BeginPropertyGrid();
                UI::PropertyColor("Radiance", dlc.Radiance);
                UI::Property("Intensity", dlc.Intensity, 0.05f, 0.f, 500.f);
                UI::Property("Radius", dlc.Radius, 0.1f, 0.f, std::numeric_limits<float>::max());
                UI::Property("Falloff", dlc.Falloff, 0.005f, 0.f, 1.f);
                UI::EndPropertyGrid();
            });

        DrawComponent<SkyLightComponent>("Sky Light", entity, [](SkyLightComponent& slc)
            {
                UI::BeginPropertyGrid();
                UI::PropertyAssetReference("Environment Map", slc.SceneEnvironment);
                UI::Property("Intensity", slc.Intensity, 0.01f, 0.0f, 5.0f);
                if (slc.SceneEnvironment)
                {
                    if (slc.SceneEnvironment->RadianceMap)
                    {
                        UI::PropertySlider("Lod", slc.Lod, 0, slc.SceneEnvironment->RadianceMap->GetMipLevelCount());
                    }
                    else
                    {
                        UI::PushItemDisabled();
                        UI::PropertySlider("Lod", slc.Lod, 0, 10);
                        UI::PopItemDisabled();
                    }
                }
                ImGui::Separator();

                UI::Property("Dynamic Sky", slc.DynamicSky);
                if (slc.DynamicSky)
                {
                    bool changed = UI::Property("Turbidity", slc.TurbidityAzimuthInclination.x, 0.01f);
                    changed |= UI::Property("Azimuth", slc.TurbidityAzimuthInclination.y, 0.01f);
                    changed |= UI::Property("Inclination", slc.TurbidityAzimuthInclination.z, 0.01f);
                    if (changed)
                    {
                        Ref<TextureCube> preethamEnv = Renderer::CreatePreethamSky(slc.TurbidityAzimuthInclination.x, slc.TurbidityAzimuthInclination.y, slc.TurbidityAzimuthInclination.z);
                        slc.SceneEnvironment = Ref<Environment>::Create(preethamEnv, preethamEnv);
                    }
                }
                UI::EndPropertyGrid();
            });

        DrawComponent<ScriptComponent>("Script", entity, [=](ScriptComponent& sc) mutable
            {
                UI::BeginPropertyGrid();

                bool isError = !ScriptEngine::ModuleExists(sc.ModuleName);
                std::string name = ScriptEngine::StripNamespace(Project::GetActive()->GetConfig().DefaultNamespace, sc.ModuleName);

                bool err = isError;
                if (err)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
                }

                if (UI::Property("Module Name", name))
                {
                    // Shutdown old script
                    if (!isError)
                    {
                        ScriptEngine::ShutdownScriptEntity(entity, sc.ModuleName);
                    }

                    if (ScriptEngine::ModuleExists(name))
                    {
                        sc.ModuleName = name;
                        isError = false;
                    }
                    else if (ScriptEngine::ModuleExists(Project::GetActive()->GetConfig().DefaultNamespace + "." + name))
                    {
                        sc.ModuleName = Project::GetActive()->GetConfig().DefaultNamespace + "." + name;
                        isError = false;
                    }
                    else
                    {
                        sc.ModuleName = name;
                        isError = true;
                    }

                    if (!isError)
                    {
                        ScriptEngine::InitScriptEntity(entity);
                    }
                }

                if (err)
                {
                    ImGui::PopStyleColor();
                }

                // Public Fields
                if (!isError)
                {
                    EntityInstanceData& entityInstanceData = ScriptEngine::GetEntityInstanceData(entity.GetSceneID(), id);
                    ScriptModuleFieldMap& moduleFieldMap = sc.ModuleFieldMap;
                    if (moduleFieldMap.find(sc.ModuleName) != moduleFieldMap.end())
                    {
                        auto& publicFields = moduleFieldMap.at(sc.ModuleName);
                        for (auto& [name, field] : publicFields)
                        {
                            bool isRuntime = mContext->mIsPlaying && entityInstanceData.Instance.IsRuntimeAvailable();
                            if (field.IsReadOnly)
                            {
                                UI::PushItemDisabled();
                            }

                            switch (field.Type)
                            {
                            case FieldType::Bool:
                            {
                                bool value = isRuntime ? field.GetRuntimeValue<bool>(entityInstanceData.Instance) : field.GetStoredValue<bool>();
                                if (UI::Property(field.Name.c_str(), value))
                                {
                                    if (isRuntime)
                                    {
                                        field.SetRuntimeValue(entityInstanceData.Instance, value);
                                    }
                                    else
                                    {
                                        field.SetStoredValue(value);
                                    }
                                }
                                break;
                            }
                            case FieldType::Int:
                            {
                                int value = isRuntime ? field.GetRuntimeValue<int>(entityInstanceData.Instance) : field.GetStoredValue<int>();
                                if (UI::Property(field.Name.c_str(), value))
                                {
                                    if (isRuntime)
                                    {
                                        field.SetRuntimeValue(entityInstanceData.Instance, value);
                                    }
                                    else
                                    {
                                        field.SetStoredValue(value);
                                    }
                                }
                                break;
                            }
                            case FieldType::String:
                            {
                                const std::string& value = isRuntime ? field.GetRuntimeValue<std::string>(entityInstanceData.Instance) : field.GetStoredValue<const std::string&>();
                                char buffer[256];
                                memset(buffer, 0, 256);
                                memcpy(buffer, value.c_str(), value.length());
                                if (UI::Property(field.Name.c_str(), buffer, 256))
                                {
                                    if (isRuntime)
                                    {
                                        field.SetRuntimeValue<const std::string&>(entityInstanceData.Instance, buffer);
                                    }
                                    else
                                    {
                                        field.SetStoredValue<const std::string&>(buffer);
                                    }
                                }
                                break;
                            }
                            case FieldType::Float:
                            {
                                float value = isRuntime ? field.GetRuntimeValue<float>(entityInstanceData.Instance) : field.GetStoredValue<float>();
                                if (UI::Property(field.Name.c_str(), value, 0.2f))
                                {
                                    if (isRuntime)
                                    {
                                        field.SetRuntimeValue(entityInstanceData.Instance, value);
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
                                glm::vec2 value = isRuntime ? field.GetRuntimeValue<glm::vec2>(entityInstanceData.Instance) : field.GetStoredValue<glm::vec2>();
                                if (UI::Property(field.Name.c_str(), value, 0.2f))
                                {
                                    if (isRuntime)
                                    {
                                        field.SetRuntimeValue(entityInstanceData.Instance, value);
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
                                glm::vec3 value = isRuntime ? field.GetRuntimeValue<glm::vec3>(entityInstanceData.Instance) : field.GetStoredValue<glm::vec3>();
                                if (UI::Property(field.Name.c_str(), value, 0.2f))
                                {
                                    if (isRuntime)
                                    {
                                        field.SetRuntimeValue(entityInstanceData.Instance, value);
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
                                glm::vec4 value = isRuntime ? field.GetRuntimeValue<glm::vec4>(entityInstanceData.Instance) : field.GetStoredValue<glm::vec4>();
                                if (UI::Property(field.Name.c_str(), value, 0.2f))
                                {
                                    if (isRuntime)
                                    {
                                        field.SetRuntimeValue(entityInstanceData.Instance, value);
                                    }
                                    else
                                    {
                                        field.SetStoredValue(value);
                                    }
                                }
                                break;
                            }
                            case FieldType::Asset:
                            {
                                UUID uuid = isRuntime ? field.GetRuntimeValue<UUID>(entityInstanceData.Instance) : field.GetStoredValue<UUID>();
                                Ref<Prefab> prefab = AssetManager::IsAssetHandleValid(uuid) ? AssetManager::GetAsset<Prefab>(uuid) : nullptr;
                                if (UI::PropertyAssetReference(field.Name.c_str(), prefab))
                                {
                                    if (isRuntime)
                                    {
                                        field.SetRuntimeValue(entityInstanceData.Instance, prefab->Handle);
                                    }
                                    else
                                    {
                                        field.SetStoredValue(prefab->Handle);
                                    }
                                }
                                break;
                            }
                            case FieldType::Entity:
                            {
                                UUID uuid = isRuntime ? field.GetRuntimeValue<UUID>(entityInstanceData.Instance) : field.GetStoredValue<UUID>();
                                Entity entity = mContext->FindEntityByID(uuid);
                                if (UI::PropertyEntityReference(field.Name.c_str(), entity))
                                {
                                    if (isRuntime)
                                    {
                                        field.SetRuntimeValue(entityInstanceData.Instance, entity.GetID());
                                    }
                                    else
                                    {
                                        field.SetStoredValue(entity.GetID());
                                    }
                                }
                            }
                            }
                            if (field.IsReadOnly)
                            {
                                UI::PopItemDisabled();
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
                UI::BeginPropertyGrid();

                // Rigidbody2D Type
                const char* rb2dTypeStrings[] = { "Static", "Dynamic", "Kinematic" };
                UI::PropertyDropdown("Type", rb2dTypeStrings, 3, (int*)&rb2dc.BodyType);

                if (rb2dc.BodyType == RigidBody2DComponent::Type::Dynamic)
                {
                    UI::BeginPropertyGrid();
                    UI::Property("Fixed Rotation", rb2dc.FixedRotation);
                    UI::EndPropertyGrid();
                }

                UI::EndPropertyGrid();
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

        DrawComponent<RigidBodyComponent>("Rigidbody", entity, [&](RigidBodyComponent& rbc)
            {
                UI::BeginPropertyGrid();
                if (UI::Property("Is Kinematic", rbc.IsKinematic))
                {
                    if (!rbc.IsKinematic && entity.HasComponent<MeshColliderComponent>())
                    {
                        auto& mcc = entity.GetComponent<MeshColliderComponent>();
                        mcc.IsConvex = true;
                    }
                }

                if (!PhysicsLayerManager::IsLayerValid(rbc.Layer))
                {
                    rbc.Layer = 0;
                }

                int layerCount = PhysicsLayerManager::GetLayerCount();
                const auto& layerNames = PhysicsLayerManager::GetLayerNames();
                UI::PropertyDropdown("Layer", layerNames, layerCount, (int*)&rbc.Layer);

                static const char* sCollisionDetectionNames[] = { "Discrete", "Continuous", "Continuous Speculative" };
                UI::PropertyDropdown("Collision Detection", sCollisionDetectionNames, 3, rbc.CollisionDetection);

                UI::BeginPropertyGrid();
                UI::Property("Mass", rbc.Mass);
                UI::Property("Linear Drag", rbc.LinearDrag);
                UI::Property("Angular Drag", rbc.AngularDrag);
                UI::Property("Disable Gravity", rbc.DisableGravity);
                UI::EndPropertyGrid();

                if (UI::BeginTreeNode("Constraints", false))
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
                    UI::EndTreeNode();
                }

                UI::EndPropertyGrid();
            });

        DrawComponent<BoxColliderComponent>("Box Collider", entity, [](BoxColliderComponent& bcc)
            {
                UI::BeginPropertyGrid();

                if (UI::Property("Size", bcc.Size))
                {
                    bcc.DebugMesh = MeshFactory::CreateBox(bcc.Size);
                }

                UI::Property("Offset", bcc.Offset);
                UI::Property("Is Trigger", bcc.IsTrigger);
                UI::PropertyAssetReference("Material", bcc.Material);

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
                UI::PropertyAssetReference("Material", scc.Material);

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
                UI::PropertyAssetReference("Material", ccc.Material);

                UI::EndPropertyGrid();
            });

        DrawComponent<MeshColliderComponent>("Mesh Collider", entity, [&](MeshColliderComponent& mcc)
            {
                UI::BeginPropertyGrid();

                bool cookMesh = false;
                if (mcc.OverrideMesh && UI::PropertyAssetReference("Mesh", mcc.CollisionMesh))
                {
                    cookMesh = true;
                }

                bool wasConvex = mcc.IsConvex;
                if (UI::Property("Is Convex", mcc.IsConvex))
                {
                    cookMesh = true;

                    if (wasConvex && !mcc.IsConvex && entity.HasComponent<RigidBodyComponent>())
                    {
                        auto& rb = entity.GetComponent<RigidBodyComponent>();
                        if (rb.BodyType == RigidBodyComponent::Type::Dynamic && !rb.IsKinematic)
                        {
                            mcc.IsConvex = true;
                            cookMesh = false;
                            NR_CORE_ERROR("MeshColliderComponent must be convex for non-kinematic dynamic Rigidbodies!");
                        }
                    }
                }

                UI::Property("Is Trigger", mcc.IsTrigger);
                UI::PropertyAssetReference("Material", mcc.Material);

                if (UI::Property("Override Mesh", mcc.OverrideMesh))
                {
                    if (!mcc.OverrideMesh && entity.HasComponent<MeshComponent>())
                    {
                        mcc.CollisionMesh = entity.GetComponent<MeshComponent>().MeshObj;
                        cookMesh = true;
                    }
                }

                if (cookMesh)
                {
                    CookingFactory::CookMesh(mcc, true);
                }

                UI::EndPropertyGrid();
            });


        DrawComponent<AudioListenerComponent>("Audio Listener", entity, [&](AudioListenerComponent& alc)
            {
                UI::BeginPropertyGrid();
                if (UI::Property("Active", alc.Active))
                {
                    auto view = mContext->GetAllEntitiesWith<AudioListenerComponent>();
                    if (alc.Active == true)
                    {
                        for (auto ent : view)
                        {
                            Entity e{ ent, mContext.Raw() };
                            e.GetComponent<AudioListenerComponent>().Active = ent == entity;
                        }
                    }
                }
                float inAngle = glm::degrees(alc.ConeInnerAngleInRadians);
                float outAngle = glm::degrees(alc.ConeOuterAngleInRadians);
                float outGain = alc.ConeOuterGain;
                //? Have to manually clamp here because UI::Property doesn't take flags to pass in ImGuiSliderFlags_ClampOnInput
                if (UI::Property("Inner Cone Angle", inAngle, 1.0f, 0.0f, 360.0f))
                {
                    if (inAngle > 360.0f)
                    {
                        inAngle = 360.0f;
                    }
                    alc.ConeInnerAngleInRadians = glm::radians(inAngle);
                }
                if (UI::Property("Outer Cone Angle", outAngle, 1.0f, 0.0f, 360.0f))
                {
                    if (outAngle > 360.0f)
                    {
                        outAngle = 360.0f;
                    }
                    alc.ConeOuterAngleInRadians = glm::radians(outAngle);
                }
                if (UI::Property("Outer Gain", outGain, 0.01f, 0.0f, 1.0f))
                {
                    if (outGain > 1.0f)
                    {
                        outGain = 1.0f;
                    }
                    alc.ConeOuterGain = outGain;
                }
                UI::EndPropertyGrid();
            });

        DrawComponent<AudioComponent>("Audio", entity, [&](AudioComponent& ac)
            {
                auto propertyGridSpacing = []
                    {
                        ImGui::Spacing();
                        ImGui::NextColumn();
                        ImGui::NextColumn();
                    };

                auto& colors = ImGui::GetStyle().Colors;
                auto oldSCol = colors[ImGuiCol_Separator];
                const float brM = 0.6f;
                colors[ImGuiCol_Separator] = ImVec4{ oldSCol.x * brM, oldSCol.y * brM, oldSCol.z * brM, 1.0f };

                UI::PushID();
                UI::BeginPropertyGrid();

                //=== Audio Objects API

                if (UI::Property("Start Event", ac.StartEvent))
                {
                    ac.StartCommandID = Audio::CommandID::FromString(ac.StartEvent.c_str());
                }

                //=====================

                if (UI::Property("Volume Multiplier", ac.VolumeMultiplier, 0.01f, 0.0f, 1.0f)) //TODO: switch to dBs in the future ?
                {
                    //ac.VolumeMultiplier = soundConfig.VolumeMultiplier;
                }
                if (UI::Property("Pitch Multiplier", ac.PitchMultiplier, 0.01f, 0.0f, 24.0f)) // max pitch 24 is just an arbitrary number here
                {
                    //ac.PitchMultiplier = soundConfig.PitchMultiplier;
                }

                UI::Property("Play on Awake", ac.PlayOnAwake);
                UI::Property("Stop When Entity Is Destroyed", ac.StopIfEntityDestroyed);

                UI::EndPropertyGrid();
                UI::PopID();
                colors[ImGuiCol_Separator] = oldSCol;
            });
                        }
                    }