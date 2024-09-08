#include "nrpch.h"
#include "AssetManagerPanel.h"

#include "NotRed/Core/Application.h"
#include "NotRed/Util/DragDropData.h"
#include "AssetEditorPanel.h"

namespace NR
{
    AssetManagerPanel::AssetManagerPanel()
    {
        AssetManager::SetAssetChangeCallback([&]()
            {
                UpdateCurrentDirectory(mCurrentDirIndex);
            });

        mFolderTex = Texture2D::Create("Assets/Editor/folder.png");
        mFavoritesTex = Texture2D::Create("Assets/Editor/favourites.png");
        mGoBackTex = Texture2D::Create("Assets/Editor/back.png");
        mScriptTex = Texture2D::Create("Assets/Editor/script.png");
        mResourceTex = Texture2D::Create("Assets/Editor/resource.png");
        mSceneTex = Texture2D::Create("Assets/Editor/scene.png");

        mAssetIconMap[-1] = Texture2D::Create("Assets/Editor/file.png");
        mAssetIconMap[AssetTypes::GetAssetTypeID("hdr")] = Texture2D::Create("Assets/Editor/file.png");
        mAssetIconMap[AssetTypes::GetAssetTypeID("obj")] = Texture2D::Create("Assets/Editor/obj.png");
        mAssetIconMap[AssetTypes::GetAssetTypeID("wav")] = Texture2D::Create("Assets/Editor/wav.png");
        mAssetIconMap[AssetTypes::GetAssetTypeID("cs")] = Texture2D::Create("Assets/Editor/csc.png");
        mAssetIconMap[AssetTypes::GetAssetTypeID("png")] = Texture2D::Create("Assets/Editor/png.png");
        mAssetIconMap[AssetTypes::GetAssetTypeID("blend")] = Texture2D::Create("Assets/Editor/blend.png");
        mAssetIconMap[AssetTypes::GetAssetTypeID("nrsc")] = Texture2D::Create("Assets/Editor/notred.png");

        mBackbtnTex = Texture2D::Create("Assets/Editor/back.png");
        mFwrdbtnTex = Texture2D::Create("Assets/Editor/btn_fwrd.png");
        mFolderRightTex = Texture2D::Create("Assets/Editor/right.png");
        mSearchTex = Texture2D::Create("Assets/Editor/search.png");
        mTagsTex = Texture2D::Create("Assets/Editor/tags.png");
        mGridView = Texture2D::Create("Assets/Editor/grid.png");
        mListView = Texture2D::Create("Assets/Editor/list.png");

        mBaseDirIndex = 0;
        mBaseProjectDir = AssetManager::GetDirectoryInfo(mBaseDirIndex);
        UpdateCurrentDirectory(mBaseDirIndex);

        memset(mInputBuffer, 0, sizeof(mInputBuffer));
    }

    void AssetManagerPanel::DrawDirectoryInfo(DirectoryInfo& dir)
    {
        if (ImGui::TreeNode(dir.DirectoryName.c_str()))
        {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                UpdateCurrentDirectory(dir.DirectoryIndex);
            }

            for (int j = 0; j < dir.ChildrenIndices.size(); ++j)
            {
                DrawDirectoryInfo(AssetManager::GetDirectoryInfo(dir.ChildrenIndices[j]));
            }

            ImGui::TreePop();
        }

        if (mIsDragging && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        {
            mMovePath = dir.FilePath;
        }
    }

    void AssetManagerPanel::ImGuiRender()
    {
        ImGui::Begin("Project", NULL, ImGuiWindowFlags_None);
        {
            UI::BeginPropertyGrid();
            ImGui::SetColumnOffset(1, 240);

            ImGui::BeginChild("##folders_common");
            {
                if (ImGui::CollapsingHeader("Content", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                {
                    for (int i = 0; i < mBaseProjectDir.ChildrenIndices.size(); ++i)
                    {
                        DrawDirectoryInfo(AssetManager::GetDirectoryInfo(mBaseProjectDir.ChildrenIndices[i]));
                    }
                }
            }
            ImGui::EndChild();

            if (ImGui::BeginDragDropTarget())
            {
                auto data = ImGui::AcceptDragDropPayload("selectable", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
                if (data)
                {
                    mIsDragging = false;
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::NextColumn();

            ImGui::BeginChild("##directory_structure", ImVec2(ImGui::GetColumnWidth() - 12, ImGui::GetWindowHeight() - 50));
            {
                ImGui::BeginChild("##directory_breadcrumbs", ImVec2(ImGui::GetColumnWidth() - 100, 30));
                RenderBreadCrumbs();
                ImGui::EndChild();

                ImGui::BeginChild("Scrolling");

                if (!mDisplayListView)
                {
                    ImGui::Columns(10, nullptr, false);
                }

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.205f, 0.21f, 0.25f));

                for (DirectoryInfo& dir : mCurrentDirChildren)
                {
                    if (mDisplayListView)
                    {
                        RenderDirectoriesListView(dir);
                    }
                    else
                    {
                        RenderDirectoriesGridView(dir);
                    }

                    ImGui::NextColumn();
                }

                for (Ref<Asset>& asset : mCurrentDirAssets)
                {
                    if (mDisplayListView)
                    {
                        RenderFileListView(asset);
                    }
                    else
                    {
                        RenderFileGridView(asset);
                    }

                    ImGui::NextColumn();
                }

                if (mIsDragging && !ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.1F))
                {
                    mIsDragging = false;
                    mDraggedAssetID = 0;
                }

                ImGui::PopStyleColor(2);

                if (ImGui::BeginPopupContextWindow(0, 1))
                {
                    if (ImGui::BeginMenu("Create"))
                    {
                        if (ImGui::MenuItem("Folder"))
                        {
                            bool created = FileSystem::CreateFolder(mCurrentDir.FilePath + "/Folder");

                            if (created)
                            {

                            }
                        }

                        if (ImGui::MenuItem("Scene"))
                        {
                        }

                        if (ImGui::MenuItem("Script"))
                        {
                        }

                        if (ImGui::MenuItem("Prefab"))
                        {
                        }

                        if (ImGui::MenuItem("Physics Material"))
                        {
                            AssetManager::CreateAsset<PhysicsMaterial>("Physics Material.hpm", AssetType::PhysicsMat, mCurrentDirIndex, 0.6f, 0.6f, 0.0f);
                            UpdateCurrentDirectory(mCurrentDirIndex);
                        }

                        ImGui::EndMenu();
                    }

                    if (ImGui::MenuItem("Import"))
                    {
                    }

                    if (ImGui::MenuItem("Refresh"))
                    {
                        UpdateCurrentDirectory(mCurrentDirIndex);
                    }

                    ImGui::EndPopup();
                }

                ImGui::EndChild();
            }
            ImGui::EndChild();

            if (ImGui::BeginDragDropTarget())
            {
                auto data = ImGui::AcceptDragDropPayload("selectable", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
                if (data)
                {
                    mIsDragging = false;
                }
                ImGui::EndDragDropTarget();
            }

            UI::EndPropertyGrid();
        }
        ImGui::End();
    }

    void AssetManagerPanel::RenderDirectoriesListView(DirectoryInfo& dirInfo)
    {
        ImGui::Image((ImTextureID)mFolderTex->GetRendererID(), ImVec2(30, 30));
        ImGui::SameLine();

        if (ImGui::Selectable(dirInfo.DirectoryName.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0, 30)))
        {
            if (ImGui::IsMouseDoubleClicked(0))
            {
                mPrevDirIndex = mCurrentDirIndex;
                UpdateCurrentDirectory(dirInfo.DirectoryIndex);
            }
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
        {
            ImGui::Image((ImTextureID)mFolderTex->GetRendererID(), ImVec2(20, 20));
            ImGui::SameLine();
            ImGui::Text(dirInfo.DirectoryName.c_str());
            ImGui::SetDragDropPayload("selectable", &dirInfo.DirectoryIndex, sizeof(int));
            mIsDragging = true;
            ImGui::EndDragDropSource();
        }
    }

    void AssetManagerPanel::RenderDirectoriesGridView(DirectoryInfo& dirInfo)
    {
        ImGui::BeginGroup();

        float columnWidth = ImGui::GetColumnWidth();
        ImGui::ImageButton((ImTextureID)mFolderTex->GetRendererID(), { columnWidth - 15.0f, columnWidth - 15.0f });

        if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
        {
            mPrevDirIndex = mCurrentDirIndex;
            UpdateCurrentDirectory(dirInfo.DirectoryIndex);
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
        {
            ImGui::Image((ImTextureID)mFolderTex->GetRendererID(), ImVec2(20, 20));
            ImGui::SameLine();
            ImGui::Text(dirInfo.DirectoryName.c_str());
            ImGui::SetDragDropPayload("selectable_directory", &dirInfo.DirectoryIndex, sizeof(int));
            mIsDragging = true;
            ImGui::EndDragDropSource();
        }

        ImVec2 prevCursorPos = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(prevCursorPos.x + 10.0f, prevCursorPos.y - 21.0f));
        ImGui::TextWrapped(dirInfo.DirectoryName.c_str());
        ImGui::SetCursorPos(prevCursorPos);

        ImGui::EndGroup();
    }

    void AssetManagerPanel::RenderFileListView(Ref<Asset>& asset)
    {
        size_t fileID = AssetTypes::GetAssetTypeID(asset->Extension);
        RendererID iconRef = mAssetIconMap[fileID]->GetRendererID();
        ImGui::Image((ImTextureID)iconRef, ImVec2(30, 30));

        ImGui::SameLine();

        ImGui::Selectable(asset->FileName.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0, 30));

        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered())
        {
            AssetEditorPanel::OpenEditor(asset);
        }

        HandleDragDrop(iconRef, asset);
    }

    void AssetManagerPanel::RenderFileGridView(Ref<Asset>& asset)
    {
        ImGui::BeginGroup();

        size_t fileID = AssetTypes::GetAssetTypeID(asset->Extension);
        fileID = mAssetIconMap.find(fileID) != mAssetIconMap.end() ? fileID : -1;
        RendererID iconRef = mAssetIconMap[fileID]->GetRendererID();
        float columnWidth = ImGui::GetColumnWidth();

        ImGui::ImageButton((ImTextureID)iconRef, { columnWidth - 15.0f, columnWidth - 15.0f });

        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered())
        {
            AssetEditorPanel::OpenEditor(asset);
        }

        HandleDragDrop(iconRef, asset);

        std::string newFileName = AssetManager::StripExtras(asset->FileName);
        ImGui::TextWrapped(newFileName.c_str());
        ImGui::EndGroup();
    }

    void AssetManagerPanel::HandleDragDrop(RendererID icon, Ref<Asset>& asset)
    {
        if (mDraggedAssetID != 0 && mDraggedAssetID != asset->Handle)
        {
            return;
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlapped) && ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            mDraggedAssetID = asset->Handle;
        }

        // Drag 'n' Drop Implementation For Asset Handling In Scene Viewport
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            ImGui::Image((ImTextureID)icon, ImVec2(20, 20));
            ImGui::SameLine();
            ImGui::Text(asset->FileName.c_str());
            ImGui::SetDragDropPayload("scene_entity_assetsP", &mDraggedAssetID, sizeof(AssetHandle));
            mIsDragging = true;

            ImGui::EndDragDropSource();
        }
    }

    void AssetManagerPanel::RenderBreadCrumbs()
    {
        RendererID viewTexture = mDisplayListView ? mListView->GetRendererID() : mGridView->GetRendererID();
        if (ImGui::ImageButton((ImTextureID)viewTexture, ImVec2(20, 18)))
        {
            mDisplayListView = !mDisplayListView;
        }

        ImGui::SameLine();

        if (ImGui::ImageButton((ImTextureID)mBackbtnTex->GetRendererID(), ImVec2(20, 18)))
        {
            if (mCurrentDirIndex == mBaseDirIndex)
            {
                return;
            }
            mNextDirIndex = mCurrentDirIndex;
            mPrevDirIndex = AssetManager::GetDirectoryInfo(mCurrentDirIndex).ParentIndex;
            UpdateCurrentDirectory(mPrevDirIndex);
        }

        ImGui::SameLine();

        if (ImGui::ImageButton((ImTextureID)mFwrdbtnTex->GetRendererID(), ImVec2(20, 18)))
        {
            UpdateCurrentDirectory(mNextDirIndex);
        }

        ImGui::SameLine();

        {
            ImGui::PushItemWidth(200);

            if (ImGui::InputTextWithHint("##", "Search...", mInputBuffer, 100))
            {
                SearchResults results = AssetManager::SearchFiles(mInputBuffer, mCurrentDir.FilePath);

                if (strlen(mInputBuffer) == 0)
                {
                    UpdateCurrentDirectory(mCurrentDirIndex);
                }
                else
                {
                    mCurrentDirChildren = results.Directories;
                    mCurrentDirAssets = results.Assets;
                }
            }

            ImGui::PopItemWidth();
        }

        ImGui::SameLine();

        if (mUpdateBreadCrumbs)
        {
            mBreadCrumbData.clear();

            int currentDirIndex = mCurrentDirIndex;
            while (currentDirIndex != -1)
            {
                DirectoryInfo& dirInfo = AssetManager::GetDirectoryInfo(currentDirIndex);
                mBreadCrumbData.push_back(dirInfo);
                currentDirIndex = dirInfo.ParentIndex;
            }

            std::reverse(mBreadCrumbData.begin(), mBreadCrumbData.end());

            mUpdateBreadCrumbs = false;
        }

        for (int i = 0; i < mBreadCrumbData.size(); i++)
        {
            if (mBreadCrumbData[i].DirectoryName != "assets")
            {
                ImGui::Text("/");
            }

            ImGui::SameLine();

            int size = strlen(mBreadCrumbData[i].DirectoryName.c_str()) * 7;

            if (ImGui::Selectable(mBreadCrumbData[i].DirectoryName.c_str(), false, 0, ImVec2(size, 22)))
            {
                UpdateCurrentDirectory(mBreadCrumbData[i].DirectoryIndex);
            }

            ImGui::SameLine();
        }

        ImGui::SameLine();

        ImGui::Dummy(ImVec2(ImGui::GetColumnWidth() - 400, 0));
        ImGui::SameLine();

    }

    void AssetManagerPanel::RenderBottom()
    {
        ImGui::BeginChild("##nav", ImVec2(ImGui::GetColumnWidth() - 12, 23));
        {
            ImGui::EndChild();
        }
    }

    void AssetManagerPanel::UpdateCurrentDirectory(int dirIndex)
    {
        if (mCurrentDirIndex != dirIndex)
        {
            mUpdateBreadCrumbs = true;
        }

        mCurrentDirChildren.clear();
        mCurrentDirAssets.clear();

        mCurrentDirIndex = dirIndex;
        mCurrentDir = AssetManager::GetDirectoryInfo(mCurrentDirIndex);

        for (int childIndex : mCurrentDir.ChildrenIndices)
        {
            mCurrentDirChildren.push_back(AssetManager::GetDirectoryInfo(childIndex));
        }

        mCurrentDirAssets = AssetManager::GetAssetsInDirectory(mCurrentDirIndex);
    }
}