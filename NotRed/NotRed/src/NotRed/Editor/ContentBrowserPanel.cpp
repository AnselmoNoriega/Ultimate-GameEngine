#include "nrpch.h"
#include "ContentBrowserPanel.h"

#include <filesystem>

#include <imgui_internal.h>

#include "NotRed/Core/Application.h"
#include "NotRed/Core/Input.h"
#include "AssetEditorPanel.h"

namespace NR
{
	static int sColumnCount = 2;

	ContentBrowserPanel::ContentBrowserPanel()
	{
		AssetManager::SetAssetChangeCallback(NR_BIND_EVENT_FN(ContentBrowserPanel::FileSystemChanged));
		mBaseDirectoryHandle = ProcessDirectory("assets", 0);

		mFileTex = AssetManager::GetAsset<Texture2D>("Assets/Editor/file.png");
		mFolderIcon = AssetManager::GetAsset<Texture2D>("Assets/Editor/folder.png");
		mAssetIconMap[""] = AssetManager::GetAsset<Texture2D>("Assets/Editor/folder.png");
		mAssetIconMap["fbx"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/fbx.png");
		mAssetIconMap["obj"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/obj.png");
		mAssetIconMap["wav"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/wav.png");
		mAssetIconMap["cs"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/csc.png");
		mAssetIconMap["png"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/png.png");
		mAssetIconMap["nrsc"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/notred.png");

		mBackbtnTex = AssetManager::GetAsset<Texture2D>("Assets/Editor/back.png");
		mFwrdbtnTex = AssetManager::GetAsset<Texture2D>("Assets/Editor/fwrd.png");

		mBaseDirectory = mDirectories[mBaseDirectoryHandle];
		
		UpdateCurrentDirectory(mBaseDirectoryHandle);

		memset(mRenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
		memset(mSearchBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
	}

	AssetHandle ContentBrowserPanel::ProcessDirectory(const std::string& directoryPath, AssetHandle parent)
	{
		Ref<DirectoryInfo> directoryInfo = Ref<DirectoryInfo>::Create();
		directoryInfo->Handle = AssetHandle();
		directoryInfo->Parent = parent;
		directoryInfo->FilePath = directoryPath;
		
		std::replace(directoryInfo->FilePath.begin(), directoryInfo->FilePath.end(), '\\', '/');
		directoryInfo->Name = Utils::GetFilename(directoryPath);

		for (auto entry : std::filesystem::directory_iterator(directoryPath))
		{
			if (entry.is_directory())
			{
				directoryInfo->SubDirectories.push_back(ProcessDirectory(entry.path().string(), directoryInfo->Handle));
				continue;
			}
		
			const auto& metadata = AssetManager::GetMetadata(entry.path().string());
			if (!metadata.IsValid())
			{
				continue;
			}
			directoryInfo->Assets.push_back(metadata.Handle);
		}

		mDirectories[directoryInfo->Handle] = directoryInfo;
		return directoryInfo->Handle;
	}

	void ContentBrowserPanel::DrawDirectoryInfo(AssetHandle directory)
	{
		const auto& dir = mDirectories[directory];

		if (ImGui::TreeNode(dir->Name.c_str()))
		{
			for (AssetHandle child : dir->SubDirectories)
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
					for (AssetHandle child : mBaseDirectory->SubDirectories)
					{
						DrawDirectoryInfo(child);
					}
				}
			}
			ImGui::EndChild();

			ImGui::NextColumn();

			ImGui::BeginChild("##directory_structure", ImVec2(0, ImGui::GetWindowHeight() - 65));
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
									const auto& createdDirectory = GetDirectoryInfo(mCurrentDirectory->FilePath + "/New Folder");
									mSelectedAssets.Select(createdDirectory->Handle);
									memset(mRenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
									memcpy(mRenameBuffer, createdDirectory->Name.c_str(), createdDirectory->Name.size());
									mRenamingSelected = true;
								}
							}

							if (ImGui::MenuItem("Physics Material"))
							{
								AssetManager::CreateNewAsset<PhysicsMaterial>("New Physics Material.hpm", mCurrentDirectory->FilePath, 0.6f, 0.6f, 0.0f);
								UpdateCurrentDirectory(mCurrentDirHandle);
							}

							ImGui::EndMenu();
						}

						if (ImGui::MenuItem("Refresh"))
						{
							UpdateCurrentDirectory(mCurrentDirHandle);
						}

						ImGui::EndPopup();
					}

					ImGui::Columns(sColumnCount, nullptr, false);

					for (auto& directory : mCurrentDirectories)
					{
						RenderDirectory(directory);
						ImGui::NextColumn();
					}

					for (auto& asset : mCurrentAssets)
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

			RenderBottomBar();

			UI::EndPropertyGrid();
		}
		ImGui::End();
	}

	void ContentBrowserPanel::RenderDirectory(Ref<DirectoryInfo>& directory)
	{
		ImGui::PushID(&directory->Handle);
		ImGui::BeginGroup();

		bool selected = mSelectedAssets.IsSelected(directory->Handle);

		if (selected)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 0.75f));
		}

		float buttonWidth = ImGui::GetColumnWidth() - 15.0f;
		UI::ImageButton(mFolderIcon->GetImage(), { buttonWidth, buttonWidth });

		if (selected)
		{
			ImGui::PopStyleColor();
		}

		HandleDragDrop(mFolderIcon->GetImage(), directory->Handle);

		if (ImGui::IsItemHovered())
		{
			mIsAnyItemHovered = true;

			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				mPrevDirHandle = mCurrentDirHandle;
				mCurrentDirHandle = directory->Handle;
				mUpdateDirectoryNextFrame = true;
			}

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !mIsDragging)
			{
				if (!Input::IsKeyPressed(KeyCode::LeftControl))
				{
					mSelectedAssets.Clear();
				}
				if (selected)
				{
					mSelectedAssets.Deselect(directory->Handle);
				}
				else
				{
					mSelectedAssets.Select(directory->Handle);
				}
			}
		}
		bool openDeleteModal = false;

		if (selected && Input::IsKeyPressed(KeyCode::Delete) && !openDeleteModal && mSelectedAssets.SelectionCount() == 1)
		{
			openDeleteModal = true;
		}

		if (ImGui::BeginPopupContextItem("ContextMenu"))
		{
			if (ImGui::MenuItem("Rename"))
			{
				mSelectedAssets.Select(directory->Handle);
				memset(mRenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
				memcpy(mRenameBuffer, directory->Name.c_str(), directory->Name.size());
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
		if (ImGui::BeginPopupModal("Delete Asset", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Are you sure you want to delete %s and everything within it?", directory->Name.c_str());
			float columnWidth = ImGui::GetContentRegionAvail().x / 4;

			ImGui::Columns(4, 0, false);

			ImGui::SetColumnWidth(0, columnWidth);
			ImGui::SetColumnWidth(1, columnWidth);
			ImGui::SetColumnWidth(2, columnWidth);
			ImGui::SetColumnWidth(3, columnWidth);

			ImGui::NextColumn();

			if (ImGui::Button("Yes", ImVec2(columnWidth, 0)))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::NextColumn();
			ImGui::SetItemDefaultFocus();

			if (ImGui::Button("No", ImVec2(columnWidth, 0)))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::NextColumn();
			ImGui::EndPopup();
		}

		if (!deleted)
		{
			ImGui::SetNextItemWidth(buttonWidth);
			if (!selected || !mRenamingSelected)
			{
				ImGui::TextWrapped(directory->Name.c_str());
			}
			if (selected)
			{
				HandleDirectoryRename(directory);
			}
		}

		ImGui::EndGroup();
		ImGui::PopID();
	}

	void ContentBrowserPanel::RenderAsset(AssetMetadata& assetInfo)
	{
		ImGui::PushID(&assetInfo.Handle);
		ImGui::BeginGroup();

		Ref<Image2D> iconRef = mAssetIconMap.find(assetInfo.Extension) != mAssetIconMap.end() ? mAssetIconMap[assetInfo.Extension]->GetImage() : mFileTex->GetImage();
		
		bool selected = mSelectedAssets.IsSelected(assetInfo.Handle);
		if (selected)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 0.75f));
		}

		float buttonWidth = ImGui::GetColumnWidth() - 15.0f;
		UI::ImageButton(iconRef, { buttonWidth, buttonWidth });

		if (selected)
		{
			ImGui::PopStyleColor();
		}

		HandleDragDrop(iconRef, assetInfo.Handle);
		if (ImGui::IsItemHovered())
		{
			mIsAnyItemHovered = true;
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				if (assetInfo.Type == AssetType::Scene)
				{
					// TODO: Open scene in viewport
				}
				else
				{
					AssetEditorPanel::OpenEditor(AssetManager::GetAsset<Asset>(assetInfo.Handle));
				}
			}

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !mIsDragging)
			{
				if (!Input::IsKeyPressed(KeyCode::LeftControl))
				{
					mSelectedAssets.Clear();
				}

				if (selected)
				{
					mSelectedAssets.Deselect(assetInfo.Handle);
				}
				else
				{
					mSelectedAssets.Select(assetInfo.Handle);
				}
			}
		}

		bool openDeleteModal = false;

		if (selected && Input::IsKeyPressed(KeyCode::Delete) && !openDeleteModal && mSelectedAssets.SelectionCount() == 1)
		{
			openDeleteModal = true;
		}

		if (ImGui::BeginPopupContextItem("ContextMenu"))
		{
			ImGui::Text(assetInfo.FilePath.c_str());
			
			ImGui::Separator();

			if (ImGui::MenuItem("Rename"))
			{
				mSelectedAssets.Select(assetInfo.Handle);
				memset(mRenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
				memcpy(mRenameBuffer, assetInfo.FileName.c_str(), assetInfo.FileName.size());
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
			ImGui::Text("Are you sure you want to delete %s?", assetInfo.FileName.c_str());

			float columnWidth = ImGui::GetContentRegionAvail().x / 4;

			ImGui::Columns(4, 0, false);

			ImGui::SetColumnWidth(0, columnWidth);
			ImGui::SetColumnWidth(1, columnWidth);
			ImGui::SetColumnWidth(2, columnWidth);
			ImGui::SetColumnWidth(3, columnWidth);

			ImGui::NextColumn();

			if (ImGui::Button("Yes", ImVec2(columnWidth, 0)))
			{
				deleted = FileSystem::DeleteFile(assetInfo.FilePath);
				if (deleted)
				{
					AssetManager::RemoveAsset(assetInfo.Handle);
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

			if (!selected || !mRenamingSelected)
			{
				ImGui::TextWrapped(assetInfo.FileName.c_str());
			}

			if (selected)
			{
				HandleAssetRename(assetInfo);
			}
		}

		ImGui::EndGroup();
		ImGui::PopID();
	}

	void ContentBrowserPanel::HandleDragDrop(Ref<Image2D> icon, AssetHandle assetHandle)
	{
		if (!mSelectedAssets.IsSelected(assetHandle) || mIsDragging)
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
			ImGui::Text("Moving");
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
			mPrevDirHandle = mCurrentDirectory->Parent;
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
					SearchResults results = Search(mSearchBuffer, mCurrentDirHandle);
					mCurrentDirectories = results.Directories;
					mCurrentAssets = results.Assets;
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
				Ref<DirectoryInfo>& dirInfo = mDirectories[currentHandle];
				mBreadCrumbData.push_back(dirInfo);
				currentHandle = dirInfo->Parent;
			}

			std::reverse(mBreadCrumbData.begin(), mBreadCrumbData.end());

			mUpdateBreadCrumbs = false;
		}

		for (const auto& directory : mBreadCrumbData)
		{
			if (directory->Name != "Assets")
			{
				ImGui::Text("/");
			}

			ImGui::SameLine();

			ImVec2 textSize = ImGui::CalcTextSize(directory->Name.c_str());
			if (ImGui::Selectable(directory->Name.c_str(), false, 0, ImVec2(textSize.x, 22)))
			{
				UpdateCurrentDirectory(directory->Handle);
			}

			ImGui::SameLine();
		}
	}

	void ContentBrowserPanel::RenderBottomBar()
	{
		ImGui::BeginChild("##panel_controls", ImVec2(ImGui::GetColumnWidth() - 12, 30), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		{
			ImGui::Separator();

			ImGui::Columns(4, 0, false);
			if (mSelectedAssets.SelectionCount() == 1)
			{
				const AssetMetadata& asset = AssetManager::GetMetadata(mSelectedAssets[0]);
				if (asset.IsValid())
				{
					ImGui::Text(asset.FilePath.c_str());
				}
				else
				{
					ImGui::Text(mDirectories[mSelectedAssets[0]]->FilePath.c_str());
				}
			}
			else if (mSelectedAssets.SelectionCount() > 1)
			{
				ImGui::Text("%d items selected", mSelectedAssets.SelectionCount());
			}
			ImGui::NextColumn();
			ImGui::NextColumn();
			ImGui::NextColumn();
			ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
			ImGui::SliderInt("##column_count", &sColumnCount, 2, 15);
		}

		ImGui::EndChild();
	}

	void ContentBrowserPanel::HandleDirectoryRename(Ref<DirectoryInfo>& dirInfo)
	{
		if (mSelectedAssets.SelectionCount() > 1)
		{
			return;
		}

		if (!mRenamingSelected && Input::IsKeyPressed(KeyCode::F2))
		{
			memset(mRenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
			memcpy(mRenameBuffer, dirInfo->Name.c_str(), dirInfo->Name.size());
			mRenamingSelected = true;
		}

		if (mRenamingSelected)
		{
			ImGui::SetKeyboardFocusHere();
			if (ImGui::InputText("##rename_dummy", mRenameBuffer, MAX_INPUT_BUFFER_LENGTH, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				std::string newFilePath = FileSystem::Rename(dirInfo->FilePath, mRenameBuffer);
				dirInfo->FilePath = newFilePath;
				dirInfo->Name = Utils::GetFilename(newFilePath);
				mRenamingSelected = false;
				mSelectedAssets.Clear();
				mUpdateDirectoryNextFrame = true;
			}
		}
	}

	void ContentBrowserPanel::HandleAssetRename(AssetMetadata& asset)
	{
		if (mSelectedAssets.SelectionCount() > 1)
		{
			return;
		}

		if (!mRenamingSelected && Input::IsKeyPressed(KeyCode::F2))
		{
			memset(mRenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
			memcpy(mRenameBuffer, asset.FileName.c_str(), asset.FileName.size());
			mRenamingSelected = true;
		}

		if (mRenamingSelected)
		{
			ImGui::SetKeyboardFocusHere();
			if (ImGui::InputText("##rename_dummy", mRenameBuffer, MAX_INPUT_BUFFER_LENGTH, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				AssetManager::Rename(asset.Handle, mRenameBuffer);
				mRenamingSelected = false;
				mSelectedAssets.Clear();
				mUpdateDirectoryNextFrame = true;
			}
		}
	}

	void ContentBrowserPanel::UpdateCurrentDirectory(AssetHandle directoryHandle)
	{
		mCurrentDirectories.clear();
		mCurrentAssets.clear();

		mSelectedAssets.Clear();

		mUpdateBreadCrumbs = true;

		mCurrentDirHandle = directoryHandle;
		mCurrentDirectory = mDirectories[directoryHandle];

		for (auto& assetHandle : mCurrentDirectory->Assets)
		{
			mCurrentAssets.push_back(AssetManager::GetMetadata(assetHandle));
		}
		
		for (auto& handle : mCurrentDirectory->SubDirectories)
		{
			mCurrentDirectories.push_back(mDirectories[handle]);
		}

		std::sort(mCurrentDirectories.begin(), mCurrentDirectories.end(), [](const Ref<DirectoryInfo>& a, const Ref<DirectoryInfo>& b)
			{
				return Utils::ToLower(a->Name) < Utils::ToLower(b->Name);
			});

		std::sort(mCurrentAssets.begin(), mCurrentAssets.end(), [](const AssetMetadata& a, const AssetMetadata& b)
			{
				return Utils::ToLower(a.FileName) < Utils::ToLower(b.FileName);
			});
	}

	void ContentBrowserPanel::FileSystemChanged(FileSystemChangedEvent e)
	{
		switch (e.Action)
		{
		case FileSystemAction::Added:
		{
			if (e.IsDirectory)
			{
				DirectoryAdded(e.FilePath);
			}
			break;
		}
		case FileSystemAction::Delete:
		{
			AssetDeleted(e);
			break;
		}
		case FileSystemAction::Modified:
		{
			break;
		}
		case FileSystemAction::Rename:
		{
			if (!e.IsDirectory)
			{
				break;
			}

			std::filesystem::path oldFilePath = e.FilePath;
			oldFilePath = oldFilePath.parent_path() / e.OldName;

			Ref<DirectoryInfo> dirInfo = GetDirectoryInfo(oldFilePath.string());
			dirInfo->FilePath = e.FilePath;
			dirInfo->Name = e.NewName;

			break;
		}
		}

		UpdateCurrentDirectory(mCurrentDirHandle);
	}

	void ContentBrowserPanel::AssetDeleted(const FileSystemChangedEvent& e)
	{
		if (!e.IsDirectory)
		{
			AssetHandle handle = AssetManager::GetAssetHandleFromFilePath(e.FilePath);
			for (auto& [dirHandle, dirInfo] : mDirectories)
			{
				for (auto it = dirInfo->Assets.begin(); it != dirInfo->Assets.end(); it++)
				{
					if (*it != handle)
					{
						continue;
					}

					dirInfo->Assets.erase(it);
					return;
				}
			}
		}
		else
		{
			RemoveDirectory(GetDirectoryInfo(e.FilePath));
		}
	}

	void ContentBrowserPanel::RemoveDirectory(Ref<DirectoryInfo> dirInfo)
	{
		if (dirInfo->Parent != 0)
		{
			auto& childList = mDirectories[dirInfo->Parent]->SubDirectories;
			childList.erase(std::remove(childList.begin(), childList.end(), dirInfo->Handle), childList.end());
		}

		for (auto subdir : dirInfo->SubDirectories)
		{
			RemoveDirectory(mDirectories[subdir]);
		}

		for (auto assetHandle : dirInfo->Assets)
		{
			AssetManager::RemoveAsset(assetHandle);
		}

		dirInfo->Assets.clear();
		dirInfo->SubDirectories.clear();
		mDirectories.erase(mDirectories.find(dirInfo->Handle));

		std::sort(mCurrentDirectories.begin(), mCurrentDirectories.end(), [](const Ref<DirectoryInfo>& a, const Ref<DirectoryInfo>& b)
			{
				return Utils::ToLower(a->Name) < Utils::ToLower(b->Name);
			});
	}

	void ContentBrowserPanel::DirectoryAdded(const std::string& directoryPath)
	{
		std::filesystem::path parentPath = directoryPath;
		parentPath = parentPath.parent_path();
		Ref<DirectoryInfo> parentInfo = GetDirectoryInfo(parentPath.string());

		if (parentInfo == nullptr)
		{
			NR_CORE_ASSERT(false, "aaaaaaaaaaaaaam...");
			return;
		}

		AssetHandle directoryHandle = ProcessDirectory(directoryPath, parentInfo->Handle);
		Ref<DirectoryInfo> directoryInfo = mDirectories[directoryHandle];
		parentInfo->SubDirectories.push_back(directoryHandle);

		for (auto& entry : std::filesystem::directory_iterator(directoryPath))
		{
			if (entry.is_directory())
			{
				DirectoryAdded(entry.path().string());
			}
			else
			{
				AssetHandle handle = AssetManager::ImportAsset(entry.path().string());
				directoryInfo->Assets.push_back(handle);
			}
		}

		std::sort(mCurrentDirectories.begin(), mCurrentDirectories.end(), [](const Ref<DirectoryInfo>& a, const Ref<DirectoryInfo>& b)
			{
				return Utils::ToLower(a->Name) < Utils::ToLower(b->Name);
			});
	}

	Ref<DirectoryInfo> ContentBrowserPanel::GetDirectoryInfo(const std::string& filepath) const
	{
		std::string fixedFilepath = filepath;
		std::replace(fixedFilepath.begin(), fixedFilepath.end(), '\\', '/');

		for (auto& [handle, directoryInfo] : mDirectories)
		{
			if (directoryInfo->FilePath == fixedFilepath)
			{
				return directoryInfo;
			}
		}

		return nullptr;
	}

	SearchResults ContentBrowserPanel::Search(const std::string& query, AssetHandle directoryHandle)
	{
		SearchResults results;
		std::string queryLowerCase = Utils::ToLower(query);
		const auto& directory = mDirectories[directoryHandle];

		for (auto& childHandle : directory->SubDirectories)
		{
			const auto& subdir = mDirectories[childHandle];
			if (subdir->Name.find(queryLowerCase) != std::string::npos)
			{
				results.Directories.push_back(subdir);
			}

			results.Append(Search(query, childHandle));
		}

		for (auto& assetHandle : directory->Assets)
		{
			const auto& asset = AssetManager::GetMetadata(assetHandle);
			std::string filename = Utils::ToLower(asset.FileName);
			if (filename.find(queryLowerCase) != std::string::npos)
			{
				results.Assets.push_back(asset);
			}
			if (queryLowerCase[0] != '.')
			{
				continue;
			}
			if (asset.Extension.find(std::string(&queryLowerCase[1])) != std::string::npos)
			{
				results.Assets.push_back(asset);
			}
		}

		return results;
	}
}