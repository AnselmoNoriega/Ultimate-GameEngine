#include "nrpch.h"
#include "AssetManagerPanel.h"

#include "NotRed/Core/Application.h"
#include "NotRed/Core/Input.h"
#include "AssetEditorPanel.h"

namespace NR
{
    static int sColumnCount = 10;

    AssetManagerPanel::AssetManagerPanel()
    {
        AssetManager::SetAssetChangeCallback([&]()
            {
                UpdateCurrentDirectory(mCurrentDirHandle);
            });

        mFileTex = AssetManager::GetAsset<Texture2D>("Assets/Editor/file.png");
        mAssetIconMap[""] = AssetManager::GetAsset<Texture2D>("Assets/Editor/folder.png");
        mAssetIconMap["fbx"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/fbx.png");
        mAssetIconMap["obj"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/obj.png");
        mAssetIconMap["wav"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/wav.png");
        mAssetIconMap["cs"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/csc.png");
        mAssetIconMap["png"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/png.png");
        mAssetIconMap["blend"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/blend.png");
        mAssetIconMap["nrc"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/notred.png");

        mBackbtnTex = AssetManager::GetAsset<Texture2D>("Assets/Editor/btn_back.png");
        mFwrdbtnTex = AssetManager::GetAsset<Texture2D>("Assets/Editor/btn_fwrd.png");
        mFolderRightTex = AssetManager::GetAsset<Texture2D>("Assets/Editor/folder_hierarchy.png");
        mSearchTex = AssetManager::GetAsset<Texture2D>("Assets/Editor/search.png");

        mBaseDirectoryHandle = AssetManager::GetAssetHandleFromFilePath("Assets");
        mBaseDirectory = AssetManager::GetAsset<Directory>(mBaseDirectoryHandle);
        UpdateCurrentDirectory(mBaseDirectoryHandle);

        memset(mInputBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
    }

    void AssetManagerPanel::DrawDirectoryInfo(AssetHandle directory)
    {
        const Ref<Directory>& dir = AssetManager::GetAsset<Directory>(directory);

        if (ImGui::TreeNode(dir->FileName.c_str()))
        {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                UpdateCurrentDirectory(directory);
            }

            for (AssetHandle child : dir->ChildDirectories)
            {
                DrawDirectoryInfo(child);
            }

            ImGui::TreePop();
        }
    }

    void AssetManagerPanel::ImGuiRender()
    {
        ImGui::Begin("Project", NULL, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
        {
            UI::BeginPropertyGrid();
            ImGui::SetColumnOffset(1, 240);

            ImGui::BeginChild("##folders_common");
            {
                if (ImGui::CollapsingHeader("Content", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                {
                    for (AssetHandle child : mBaseDirectory->ChildDirectories)
                    {
                        DrawDirectoryInfo(child);
                    }
                }
            }
            ImGui::EndChild();

            ImGui::NextColumn();

            ImGui::BeginChild("##directory_structure", ImVec2(ImGui::GetColumnWidth() - 12, ImGui::GetWindowHeight() - 60));
            {
                ImGui::BeginChild("##directory_breadcrumbs", ImVec2(ImGui::GetColumnWidth() - 100, 30));
                RenderBreadCrumbs();
                ImGui::EndChild();

                ImGui::BeginChild("Scrolling");

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.205f, 0.21f, 0.25f));

                if (Input::IsKeyPressed(KeyCode::Escape) || (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !mIsAnyItemHovered))
                {
                    mSelectedAssets.Clear();

                    mRenamingSelected = false;
                    memset(mInputBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
                }

                mIsAnyItemHovered = false;

                ImGui::PopStyleColor(2);

                if (ImGui::BeginPopupContextWindow(0, 1))
                {
                    if (ImGui::BeginMenu("Create"))
                    {
                        if (ImGui::MenuItem("Folder"))
                        {
                            bool created = FileSystem::CreateFolder(mCurrentDirectory->FilePath + "/Folder");

                            if (created)
                            {
                                UpdateCurrentDirectory(mCurrentDirHandle);
                                const auto& createdDirectory = AssetManager::GetAsset<Directory>(AssetManager::GetAssetHandleFromFilePath(mCurrentDirectory->FilePath + "/Folder"));
                                mSelectedAssets.Select(createdDirectory->Handle);
                                memset(mInputBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
                                memcpy(mInputBuffer, createdDirectory->FileName.c_str(), createdDirectory->FileName.size());
                                mRenamingSelected = true;
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
                            AssetManager::CreateAsset<PhysicsMaterial>("New Physics Material.hpm", AssetType::PhysicsMat, mCurrentDirHandle, 0.6f, 0.6f, 0.0f);
                            UpdateCurrentDirectory(mCurrentDirHandle);
                        }

                        ImGui::EndMenu();
                    }

                    if (ImGui::MenuItem("Import"))
                    {
                    }

                    if (ImGui::MenuItem("Refresh"))
                    {
                        UpdateCurrentDirectory(mCurrentDirHandle);
                    }

                    ImGui::EndPopup();
                }

                ImGui::Columns(sColumnCount, nullptr, false);

                for (Ref<Asset>& asset : mCurrentDirAssets)
                {
                    RenderAsset(asset);

                    ImGui::NextColumn();
                }

                if (mUpdateDirectoryNextFrame)
                {
                    UpdateCurrentDirectory(mCurrentDirHandle);
                    mUpdateDirectoryNextFrame = false;
                }

                if (mIsDragging && !ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.1f))
                {
                    mIsDragging = false;
                    mDraggedAssetID = 0;
                }

                ImGui::PopStyleColor(2);
                ImGui::EndChild();
            }

            ImGui::EndChild();

            ImGui::BeginChild("##panel_controls", ImVec2(ImGui::GetColumnWidth() - 12, 20), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            ImGui::Columns(4, 0, false);
            ImGui::NextColumn();

            ImGui::NextColumn();

            ImGui::NextColumn();
            ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
            ImGui::SliderInt("##column_count", &sColumnCount, 2, 15);
            ImGui::EndChild();

            UI::EndPropertyGrid();
        }

        ImGui::End();
    }

    void AssetManagerPanel::RenderAsset(Ref<Asset>& asset)
    {
        AssetHandle assetHandle = asset->Handle;
        std::string filename = asset->FileName;

        ImGui::PushID(&asset->Handle);
        ImGui::BeginGroup();

        RendererID iconRef = mAssetIconMap.find(asset->Extension) != mAssetIconMap.end() ? mAssetIconMap[asset->Extension]->GetRendererID() : mFileTex->GetRendererID();

        if (mSelectedAssets.IsSelected(assetHandle))
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 0.75f));
        }

        float buttonWidth = ImGui::GetColumnWidth() - 15.0f;
        ImGui::ImageButton((ImTextureID)iconRef, { buttonWidth, buttonWidth });

        if (mSelectedAssets.IsSelected(assetHandle))
        {
            ImGui::PopStyleColor();
        }

        HandleDragDrop(iconRef, asset);

        if (ImGui::IsItemHovered())
        {
            mIsAnyItemHovered = true;

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                if (asset->Type == AssetType::Directory)
                {
                    mPrevDirHandle = mCurrentDirHandle;
                    mCurrentDirHandle = assetHandle;
                    mUpdateDirectoryNextFrame = true;
                }
                else if (asset->Type == AssetType::Scene)
                {
                }
                else
                {
                    AssetEditorPanel::OpenEditor(asset);
                }
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !mIsDragging)
            {
                if (!Input::IsKeyPressed(KeyCode::LeftControl))
                {
                    mSelectedAssets.Clear();
                }

                if (mSelectedAssets.IsSelected(assetHandle))
                {
                    mSelectedAssets.Deselect(assetHandle);
                }
                else
                {
                    mSelectedAssets.Select(assetHandle);
                }
            }
        }

        bool shouldOpenDeleteModal = false;
        
		if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Rename"))
            {
                mSelectedAssets.Select(assetHandle);
                memset(mInputBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
                memcpy(mInputBuffer, filename.c_str(), filename.size());
                mRenamingSelected = true;
            }

            if (ImGui::MenuItem("Delete"))
            {
                shouldOpenDeleteModal = true;
            }

            ImGui::EndPopup();
        }

        if (shouldOpenDeleteModal)
        {
            ImGui::OpenPopup("Delete Asset");
            shouldOpenDeleteModal = false;
        }

        bool deleted = false;
        if (ImGui::BeginPopupModal("Delete File", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (asset->Type == AssetType::Directory)
            {
                ImGui::Text("Are you sure you want to delete %s and everything within it?", filename.c_str());
            }
            else
            {
                ImGui::Text("Are you sure you want to delete %s?", filename.c_str());
            }
            float columnWidth = ImGui::GetContentRegionAvail().x / 4;

            ImGui::Columns(4, 0, false);
            ImGui::SetColumnWidth(0, columnWidth);
            ImGui::SetColumnWidth(1, columnWidth);
            ImGui::SetColumnWidth(2, columnWidth);
            ImGui::SetColumnWidth(3, columnWidth);
            ImGui::NextColumn();
            if (ImGui::Button("Yes", ImVec2(columnWidth, 0)))
            {
                std::string filepath = asset->FilePath;
                deleted = FileSystem::DeleteFile(filepath);
                if (deleted)
                {
                    FileSystem::DeleteFile(filepath + ".meta");
                    AssetManager::RemoveAsset(assetHandle);
                    mUpdateDirectoryNextFrame = true;
                }

                ImGui::CloseCurrentPopup();
            }

            ImGui::NextColumn();
            ImGui::SetItemDefaultFocus();
            if (ImGui::Button("No", ImVec2(columnWidth, 0)))
                ImGui::CloseCurrentPopup();

            ImGui::NextColumn();
            ImGui::EndPopup();
        }

        if (!deleted)
        {
            ImGui::SetNextItemWidth(buttonWidth);

            if (!mSelectedAssets.IsSelected(assetHandle) || !mRenamingSelected)
            {
                ImGui::TextWrapped(filename.c_str());
            }

            if (mSelectedAssets.IsSelected(assetHandle))
            {
                HandleRenaming(asset);
            }
        }

        ImGui::EndGroup();
        ImGui::PopID();
    }

    void AssetManagerPanel::HandleDragDrop(RendererID icon, Ref<Asset>& asset)
    {
        if (asset->Type == AssetType::Directory && mIsDragging)
        {
            if (ImGui::BeginDragDropTarget())
            {
                auto payload = ImGui::AcceptDragDropPayload("asset_payload");
                if (payload)
                {
                    int count = payload->DataSize / sizeof(AssetHandle);

                    for (int i = 0; i < count; i++)
                    {
                        AssetHandle handle = *(((AssetHandle*)payload->Data) + i);
                        Ref<Asset> droppedAsset = AssetManager::GetAsset<Asset>(handle, false);

                        bool result = FileSystem::MoveFile(droppedAsset->FilePath, asset->FilePath);
                        if (result)
                            droppedAsset->ParentDirectory = asset->Handle;
                    }

                    mUpdateDirectoryNextFrame = true;
                }
            }
        }

        if (!mSelectedAssets.IsSelected(asset->Handle) || mIsDragging)
        {
            return;
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlapped) && ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            mIsDragging = true;
        }

        // Drag 'n' Drop Implementation For Asset Handling In Scene Viewport
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            ImGui::Image((ImTextureID)icon, ImVec2(20, 20));
            ImGui::SameLine();
            ImGui::Text(asset->FileName.c_str());
            ImGui::SetDragDropPayload("asset_payload", mSelectedAssets.GetSelectionData(), mSelectedAssets.SelectionCount() * sizeof(AssetHandle));
            mIsDragging = true;

            ImGui::EndDragDropSource();
        }
    }

    void AssetManagerPanel::RenderBreadCrumbs()
    {
        if (ImGui::ImageButton((ImTextureID)mBackbtnTex->GetRendererID(), ImVec2(20, 18)))
        {
            if (mCurrentDirHandle == mBaseDirectoryHandle) return;

            mNextDirHandle = mCurrentDirHandle;
            mPrevDirHandle = mCurrentDirectory->ParentDirectory;
            UpdateCurrentDirectory(mPrevDirHandle);
        }

        ImGui::SameLine();

        if (ImGui::ImageButton((ImTextureID)mFwrdbtnTex->GetRendererID(), ImVec2(20, 18)))
        {
            UpdateCurrentDirectory(mNextDirHandle);
        }

        ImGui::SameLine();

        {
            char* buf = mInputBuffer;
            if (mRenamingSelected)
            {
                buf = (char*)"\0";
            }

            if (ImGui::InputTextWithHint("", "Search...", buf, MAX_INPUT_BUFFER_LENGTH))
            {
                if (strlen(mInputBuffer) == 0)
                {
                    UpdateCurrentDirectory(mCurrentDirHandle);
                }
                else
                {
                    mCurrentDirAssets = AssetManager::SearchFiles(mInputBuffer, mCurrentDirectory->FilePath);
                }
            }

            ImGui::PopItemWidth();
        }

        ImGui::SameLine();

        if (mUpdateBreadCrumbs)
        {
            mBreadCrumbData.clear();

            AssetHandle currentHandle = mCurrentDirHandle;
            while (currentHandle != 0)
            {
                const Ref<Directory>& dirInfo = AssetManager::GetAsset<Directory>(currentHandle);
                mBreadCrumbData.push_back(dirInfo);
                currentHandle = dirInfo->ParentDirectory;
            }

            std::reverse(mBreadCrumbData.begin(), mBreadCrumbData.end());

            mUpdateBreadCrumbs = false;
        }

        for (int i = 0; i < mBreadCrumbData.size(); ++i)
        {
            if (mBreadCrumbData[i]->FileName != "Assets")
            {
                ImGui::Text("/");
            }

            ImGui::SameLine();

            int size = strlen(mBreadCrumbData[i]->FileName.c_str()) * 7;

            if (ImGui::Selectable(mBreadCrumbData[i]->FileName.c_str(), false, 0, ImVec2(size, 22)))
            {
                UpdateCurrentDirectory(mBreadCrumbData[i]->Handle);
            }

            ImGui::SameLine();
        }

        ImGui::SameLine();

        ImGui::Dummy(ImVec2(ImGui::GetColumnWidth() - 400, 0));
        ImGui::SameLine();

    }

    void AssetManagerPanel::HandleRenaming(Ref<Asset>& asset)
    {
        if (mSelectedAssets.SelectionCount() > 1)
        {
            return;
        }

        if (!mRenamingSelected && Input::IsKeyPressed(KeyCode::F2))
        {
            memset(mInputBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
            memcpy(mInputBuffer, asset->FileName.c_str(), asset->FileName.size());
            mRenamingSelected = true;
        }

        if (mRenamingSelected)
        {
            ImGui::SetKeyboardFocusHere();
            if (ImGui::InputText("##rename_dummy", mInputBuffer, MAX_INPUT_BUFFER_LENGTH, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                NR_CORE_INFO("Renaming to {0}", mInputBuffer);
                AssetManager::Rename(asset, mInputBuffer);
                mRenamingSelected = false;
                mSelectedAssets.Clear();
                mUpdateDirectoryNextFrame = true;
            }
        }
    }

    void AssetManagerPanel::UpdateCurrentDirectory(AssetHandle directoryHandle)
    {
        mUpdateBreadCrumbs = true;
        mCurrentDirAssets.clear();

        mCurrentDirHandle = directoryHandle;
        mCurrentDirectory = AssetManager::GetAsset<Directory>(mCurrentDirHandle);
        mCurrentDirAssets = AssetManager::GetAssetsInDirectory(mCurrentDirHandle);
    }
}