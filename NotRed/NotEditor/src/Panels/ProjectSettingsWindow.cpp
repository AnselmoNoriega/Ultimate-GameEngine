#include "ProjectSettingsWindow.h"

#include "NotRed/ImGui/ImGui.h"

#include "NotRed/Project/ProjectSerializer.h"
#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Physics/PhysicsManager.h"
#include "NotRed/Physics/PhysicsLayer.h"
#include "NotRed/Core/Input.h"

namespace NR
{
    ImFont* gBoldFont = nullptr;

    static bool sSerializeProject = false;

    ProjectSettingsWindow::ProjectSettingsWindow(const Ref<Project>& project)
        : mProject(project)
    {
        mDefaultScene = AssetManager::GetAsset<Asset>(project->GetConfig().StartScene);
        memset(mNewLayerNameBuffer, 0, 255);
    }

    ProjectSettingsWindow::~ProjectSettingsWindow()
    {
    }

    void ProjectSettingsWindow::ImGuiRender(bool& show)
    {
        if (!show)
        {
            return;
        }

        ImGui::Begin("Project Settings", &show);
        RenderGeneralSettings();
        RenderScriptingSettings();
        RenderPhysicsSettings();
        ImGui::End();

        if (sSerializeProject)
        {
            ProjectSerializer serializer(mProject);
            serializer.Serialize(mProject->mConfig.ProjectDirectory + "/" + mProject->mConfig.ProjectFileName);
            sSerializeProject = false;
        }
    }

    void ProjectSettingsWindow::RenderGeneralSettings()
    {
        ImGui::PushID("GeneralSettings");
        if (UI::PropertyGridHeader("General"))
        {
            UI::BeginPropertyGrid();
            UI::PushItemDisabled();

            UI::Property("Name", mProject->mConfig.Name);
            UI::Property("Asset Directory", mProject->mConfig.AssetDirectory);
            UI::Property("Asset Registry Path", mProject->mConfig.AssetRegistryPath);
            UI::Property("Audio Commands Registry Path", mProject->mConfig.AudioCommandsRegistryPath);
            UI::Property("Mesh Path", mProject->mConfig.MeshPath);
            UI::Property("Mesh Source Path", mProject->mConfig.MeshSourcePath);
            UI::Property("Script Module Path", mProject->mConfig.ScriptModulePath);
            UI::Property("Project Directory", mProject->mConfig.ProjectDirectory);

            UI::PopItemDisabled();

            if (UI::PropertyAssetReference("Startup Scene", mDefaultScene))
            {
                const auto& metadata = AssetManager::GetMetadata(mDefaultScene);
                if (metadata.IsValid())
                {
                    mProject->mConfig.StartScene = metadata.FilePath.string();
                    sSerializeProject = true;
                }
            }

            UI::EndPropertyGrid();
            ImGui::TreePop();
        }
        else
        {
            UI::ShiftCursorY(-(ImGui::GetStyle().ItemSpacing.y + 1.0f));
        }

        ImGui::PopID();
    }

    void ProjectSettingsWindow::RenderScriptingSettings()
    {
        ImGui::PushID("ScriptingSettings");
        if (UI::PropertyGridHeader("Scripting", false))
        {
            UI::BeginPropertyGrid();
            if (UI::Property("Default Namespace", mProject->mConfig.DefaultNamespace))
            {
                sSerializeProject = true;
            }
            if (UI::Property("Reload Assembly On Play", mProject->mConfig.ReloadAssemblyOnPlay))
            {
                sSerializeProject = true;
            }
            UI::EndPropertyGrid();
            ImGui::TreePop();
        }
        else
        {
            UI::ShiftCursorY(-(ImGui::GetStyle().ItemSpacing.y + 1.0f));
        }

        ImGui::PopID();
    }

    void ProjectSettingsWindow::RenderPhysicsSettings()
    {
        ImGui::PushID("PhysicsSettings");
        if (UI::PropertyGridHeader("Physics", false))
        {
            UI::BeginPropertyGrid();

            {
                PhysicsSettings& settings = PhysicsManager::GetSettings();

                UI::Property("Fixed Timestep (Default: 0.02)", settings.FixedDeltaTime);
                UI::Property("Gravity (Default: -9.81)", settings.Gravity.y);

                static const char* broadphaseTypeStrings[] = { "Sweep And Prune", "Multi Box Pruning", "Automatic Box Pruning" };
                UI::PropertyDropdown("Broadphase Type", broadphaseTypeStrings, 3, (int*)&settings.BroadphaseAlgorithm);

                if (settings.BroadphaseAlgorithm != BroadphaseType::AutomaticBoxPrune)
                {
                    UI::Property("World Bounds (Min)", settings.WorldBoundsMin);
                    UI::Property("World Bounds (Max)", settings.WorldBoundsMax);
                    UI::PropertySlider("Grid Subdivisions", (int&)settings.WorldBoundsSubdivisions, 1, 10000);
                }

                static const char* frictionTypeStrings[] = { "Patch", "One Directional", "Two Directional" };
                UI::PropertyDropdown("Friction Model", frictionTypeStrings, 3, (int*)&settings.FrictionModel);

                UI::PropertySlider("Solver Iterations", (int&)settings.SolverIterations, 1, 512);
                UI::PropertySlider("Solver Velocity Iterations", (int&)settings.SolverVelocityIterations, 1, 512);

#ifdef NR_DEBUG
                UI::Property("Debug On Play", settings.DebugOnPlay);

                static const char* debugTypeStrings[] = { "Debug To File", "Live Debug" };
                UI::PropertyDropdown("Debug Type", debugTypeStrings, 2, (int*)&settings.DebugType);
#endif

                ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::Theme::backgroundDark);
                ImGui::PushStyleColor(ImGuiCol_Border, Colors::Theme::backgroundDark);
                auto singleColumnSeparator = []
                    {
                        ImDrawList* draw_list = ImGui::GetWindowDrawList();
                        ImVec2 p = ImGui::GetCursorScreenPos();
                        draw_list->AddLine(ImVec2(p.x - 9999, p.y), ImVec2(p.x + 9999, p.y), ImGui::GetColorU32(ImGuiCol_Border));
                    };

                ImGui::Spacing();
                auto GetUniqueName = []()
                    {
                        int counter = 0;
                        auto checkID = [&counter](auto checkID) -> std::string
                            {
                                ++counter;
                                const std::string counterStr = [&counter] {
                                    if (counter < 10)
                                        return "0" + std::to_string(counter);
                                    else
                                        return std::to_string(counter);
                                    }();
                                std::string idstr = "NewLayer_" + counterStr;
                                if (PhysicsLayerManager::GetLayer(idstr).IsValid())
                                    return checkID(checkID);
                                else
                                    return idstr;
                            };
                        return checkID(checkID);
                    };
                bool renameSelectedLayer = false;

                if (ImGui::Button("New Layer", { 80.0f, 28.0f }))
                {
                    std::string name = GetUniqueName();
                    mSelectedLayer = PhysicsLayerManager::AddLayer(name);
                    renameSelectedLayer = true;
                    sSerializeProject = true;
                }

                ImGui::BeginChild("LayersList");
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 8.0f, 0.0f });

                for (const auto& layer : PhysicsLayerManager::GetLayers())
                {
                    ImGui::PushID(layer.Name.c_str());
                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf;
                    bool selected = mSelectedLayer == layer.ID;
                    if (selected)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, Colors::Theme::backgroundDark);
                        flags |= ImGuiTreeNodeFlags_Selected;
                    }

                    if (ImGui::TreeNodeEx(layer.Name.c_str(), flags))
                    {
                        ImGui::TreePop();
                    }

                    bool itemClicked = ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ImGui::IsItemHovered(ImGuiHoveredFlags_None) &&
                        ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 1.0f).x == 0.0f &&
                        ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 1.0f).y == 0.0f;

                    if (itemClicked)
                    {
                        mSelectedLayer = layer.ID;
                    }

                    if (selected)
                    {
                        ImGui::PopStyleColor();
                    }

                    if (layer.Name != "Default" && ImGui::BeginPopupContextItem())
                    {
                        if (ImGui::MenuItem("Rename", "F2"))
                        {
                            mSelectedLayer = layer.ID;
                            renameSelectedLayer = true;
                        }
                        if (ImGui::MenuItem("Delete", "Delete"))
                        {
                            if (selected)
                            {
                                mSelectedLayer = -1;
                            }

                            PhysicsLayerManager::RemoveLayer(layer.ID);
                            sSerializeProject = true;
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::PopID();
                }

                if (Input::IsKeyPressed(KeyCode::F2) && mSelectedLayer != -1)
                {
                    renameSelectedLayer = true;
                }

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
                {
                    mSelectedLayer = -1;
                }

                ImGui::PopStyleVar();
                ImGui::EndChild();
                ImGui::PopStyleColor(2);
                ImGui::NextColumn();

                if (mSelectedLayer != -1)
                {
                    PhysicsLayer& layerInfo = PhysicsLayerManager::GetLayer(mSelectedLayer);
                    auto propertyGridSpacing = []
                        {
                            ImGui::Spacing();
                            ImGui::NextColumn();
                            ImGui::NextColumn();
                        };
                    ImGui::Spacing();
                    ImGui::Spacing();
                    ImGui::Spacing();
                    ImGui::Spacing();
                    ImGui::Spacing();

                    singleColumnSeparator();
                    ImGui::Spacing();
                    ImGui::Spacing();

                    static char buffer[256];
                    strcpy_s(buffer, layerInfo.Name.c_str());
                    ImGui::Text("Layer Name: ");
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0.08f,0.08f,0.08f,1.0f });
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3.0f);

                    ImGuiInputTextFlags text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll;

                    if (layerInfo.Name == "Default")
                    {
                        text_flags |= ImGuiInputTextFlags_ReadOnly;
                    }

                    ImGui::PushID("selected_layer_name");

                    if (renameSelectedLayer)
                    {
                        ImGui::SetKeyboardFocusHere();
                    }

                    if (ImGui::InputText("##selected_layer_name", buffer, 256, text_flags))
                    {
                        PhysicsLayerManager::UpdateLayerName(layerInfo.ID, buffer);
                        sSerializeProject = true;
                    }

                    ImGui::PopID();
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
                    ImGui::PopStyleColor();

                    const float width = ImGui::GetColumnWidth();
                    const float borderOffset = 7.0f;
                    const float bottomAreaHeight = 50.0f;

                    auto layersBounds = ImGui::GetContentRegionAvail();
                    layersBounds.x = width - borderOffset;
                    layersBounds.y -= bottomAreaHeight;

                    ImGui::BeginChild("Collides With", layersBounds, false);
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, 1.0f });
                    ImGui::Dummy({ width, 0.0f }); // 1px space offset

                    UI::BeginPropertyGrid();
                    for (const auto& layer : PhysicsLayerManager::GetLayers())
                    {
                        if (layer.ID == mSelectedLayer)
                        {
                            continue;
                        }

                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 1.0f); // adding 1px "border"

                        bool shouldCollide = layerInfo.CollidesWith & layer.BitValue;
                        if (UI::Property(layer.Name.c_str(), shouldCollide))
                        {
                            PhysicsLayerManager::SetLayerCollision(mSelectedLayer, layer.ID, shouldCollide);
                            sSerializeProject = true;
                        }
                    }

                    UI::EndPropertyGrid();
                    ImGui::PopStyleVar();
                    ImGui::EndChild();
                }
            }
            UI::EndPropertyGrid();

            ImGui::TreePop();
        }
        else
        {
            UI::ShiftCursorY(-(ImGui::GetStyle().ItemSpacing.y + 1.0f));
        }

        ImGui::PopID();
    }
}
