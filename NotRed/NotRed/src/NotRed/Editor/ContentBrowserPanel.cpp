#include "nrpch.h"
#include "ContentBrowserPanel.h"

#include <imgui_internal.h>

#include "NotRed/Core/Application.h"
#include "NotRed/Core/Input.h"
#include "AssetEditorPanel.h"

namespace NR
{
	static int sColumnCount = 10;

	ContentBrowserPanel::ContentBrowserPanel()
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
		mAssetIconMap["nrsc"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/notred.png");

		mBackbtnTex = AssetManager::GetAsset<Texture2D>("assets/editor/back.png");
		mFwrdbtnTex = AssetManager::GetAsset<Texture2D>("assets/editor/fwrd.png");

		mBaseDirectoryHandle = AssetManager::GetAssetHandleFromFilePath("Assets");
		mBaseDirectory = AssetManager::GetAsset<Directory>(mBaseDirectoryHandle);
		UpdateCurrentDirectory(mBaseDirectoryHandle);

		memset(mRenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
		memset(mSearchBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
	}

	void ContentBrowserPanel::DrawDirectoryInfo(AssetHandle directory)
	{
		const Ref<Directory>& dir = AssetManager::GetAsset<Directory>(directory);

		if (ImGui::TreeNode(dir->FileName.c_str()))
		{
			for (AssetHandle child : dir->ChildDirectories)
			{
				DrawDirectoryInfo(child);
			}

			ImGui::TreePop();
		}

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			UpdateCurrentDirectory(directory);
		}
	}

	void ContentBrowserPanel::ImGuiRender()
	{
		ImGui::Begin("Content Browser", NULL, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
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

			ImGui::BeginChild("##directory_structure", ImVec2(0, ImGui::GetWindowHeight() - 60));
			{
				ImGui::BeginChild("##top_bar", ImVec2(0, 30));
				{
					RenderBreadCrumbs();
				}
				ImGui::EndChild();

				ImGui::Separator();

				ImGui::BeginChild("Scrolling");
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.35f));

					if (Input::IsKeyPressed(KeyCode::Escape) || (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !mIsAnyItemHovered))
					{
						mSelectedAssets.Clear();
						mRenamingSelected = false;
						memset(mRenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
						memset(mSearchBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
					}

					mIsAnyItemHovered = false;

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
									memset(mRenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
									memcpy(mRenameBuffer, createdDirectory->FileName.c_str(), createdDirectory->FileName.size());
									mRenamingSelected = true;
								}
							}

							if (ImGui::MenuItem("Scene"))
							{
								//TODO:
							}

							if (ImGui::MenuItem("Script"))
							{
								//TODO:
							}

							if (ImGui::MenuItem("Prefab"))
							{
								//TODO:
							}

							if (ImGui::MenuItem("Physics Material"))
							{
								AssetManager::CreateNewAsset<PhysicsMaterial>("PhysicsMaterial.nrpm", AssetType::PhysicsMat, mCurrentDirHandle, 0.6f, 0.6f, 0.0f);
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

					for (Ref<Asset>& asset : mCurrentDirFolders)
					{
						RenderAsset(asset);
						ImGui::NextColumn();
					}

					for (Ref<Asset>& asset : mCurrentDirFiles)
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
					}

					ImGui::PopStyleColor(2);
				}
				ImGui::EndChild();
			}
			ImGui::EndChild();

			ImGui::BeginChild("##panel_controls", ImVec2(ImGui::GetColumnWidth() - 12, 20), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			{
				ImGui::Columns(4, 0, false);
				ImGui::NextColumn();
				ImGui::NextColumn();
				ImGui::NextColumn();
				ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
				ImGui::SliderInt("##column_count", &sColumnCount, 2, 15);
			}
			ImGui::EndChild();

			UI::EndPropertyGrid();
		}
		ImGui::End();
	}

	void ContentBrowserPanel::RenderAsset(Ref<Asset>& asset)
	{
		AssetHandle assetHandle = asset->Handle;
		std::string filename = asset->FileName;

		ImGui::PushID(&asset->Handle);
		ImGui::BeginGroup();

		Ref<Image2D> iconRef = mAssetIconMap.find(asset->Extension) != mAssetIconMap.end() ? mAssetIconMap[asset->Extension]->GetImage() : mFileTex->GetImage();

		if (mSelectedAssets.IsSelected(assetHandle))
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 0.75f));
		}

		float buttonWidth = ImGui::GetColumnWidth() - 15.0f;
		UI::ImageButton(iconRef, { buttonWidth, buttonWidth });

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

		bool openDeleteModal = false;

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Rename"))
			{
				mSelectedAssets.Select(assetHandle);			
				memset(mRenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
				memcpy(mRenameBuffer, filename.c_str(), filename.size());
				mRenamingSelected = true;
			}

			if (ImGui::MenuItem("Delete"))
			{
				openDeleteModal = true;
			}

			ImGui::EndPopup();
		}

		if (openDeleteModal)
		{
			ImGui::OpenPopup("Delete Asset");
			openDeleteModal = false;
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

	void ContentBrowserPanel::HandleDragDrop(Ref<Image2D> icon, Ref<Asset>& asset)
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
			UI::Image(icon, ImVec2(20, 20));
			ImGui::SameLine();
			ImGui::Text(asset->FileName.c_str());
			ImGui::SetDragDropPayload("asset_payload", mSelectedAssets.GetSelectionData(), mSelectedAssets.SelectionCount() * sizeof(AssetHandle));
			mIsDragging = true;

			ImGui::EndDragDropSource();
		}
	}

	void ContentBrowserPanel::RenderBreadCrumbs()
	{
		if (UI::ImageButton(mBackbtnTex->GetImage(), ImVec2(20, 18)))
		{
			if (mCurrentDirHandle == mBaseDirectoryHandle) return;

			mNextDirHandle = mCurrentDirHandle;
			mPrevDirHandle = mCurrentDirectory->ParentDirectory;
			UpdateCurrentDirectory(mPrevDirHandle);
		}

		ImGui::SameLine();

		if (UI::ImageButton(mFwrdbtnTex->GetImage(), ImVec2(20, 18)))
		{
			UpdateCurrentDirectory(mNextDirHandle);
		}

		ImGui::SameLine();

		{
			ImGui::PushItemWidth(200);			
			
			if (ImGui::InputTextWithHint("##", "Search...", mSearchBuffer, MAX_INPUT_BUFFER_LENGTH))
			{
				if (strlen(mSearchBuffer) == 0)
				{
					UpdateCurrentDirectory(mCurrentDirHandle);
				}
				else
				{
					mCurrentDirFolders = AssetManager::SearchAssets(mSearchBuffer, mCurrentDirectory->FilePath, AssetType::Directory);
					mCurrentDirFiles = AssetManager::SearchAssets(mSearchBuffer, mCurrentDirectory->FilePath);
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

	void ContentBrowserPanel::HandleRenaming(Ref<Asset>& asset)
	{
		if (mSelectedAssets.SelectionCount() > 1)
		{
			return;
		}

		if (!mRenamingSelected && Input::IsKeyPressed(KeyCode::F2))
		{
			memset(mRenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
			memcpy(mRenameBuffer, asset->FileName.c_str(), asset->FileName.size());
			mRenamingSelected = true;
		}

		if (mRenamingSelected)
		{
			ImGui::SetKeyboardFocusHere();
			if (ImGui::InputText("##rename_dummy", mRenameBuffer, MAX_INPUT_BUFFER_LENGTH, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				NR_CORE_INFO("Renaming to {0}", mRenameBuffer);
				AssetManager::Rename(asset->Handle, mRenameBuffer);
				mRenamingSelected = false;
				mSelectedAssets.Clear();
				mUpdateDirectoryNextFrame = true;
			}
		}
	}

	void ContentBrowserPanel::UpdateCurrentDirectory(AssetHandle directoryHandle)
	{
		mUpdateBreadCrumbs = true;		
		mCurrentDirFiles.clear();
		mCurrentDirFolders.clear();

		mCurrentDirHandle = directoryHandle;
		mCurrentDirectory = AssetManager::GetAsset<Directory>(mCurrentDirHandle);

		std::vector<Ref<Asset>> assets = AssetManager::GetAssetsInDirectory(mCurrentDirHandle);
		for (auto& asset : assets)
		{
			if (asset->Type == AssetType::Directory)
			{
				mCurrentDirFolders.push_back(asset);
			}
			else
			{
				mCurrentDirFiles.push_back(asset);
			}
		}
	}
}