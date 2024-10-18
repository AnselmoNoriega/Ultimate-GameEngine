#include "nrpch.h"
#include "ContentBrowserPanel.h"

#include <filesystem>
#include <stack>

#include <imgui_internal.h>

#include "NotRed/Core/Application.h"
#include "NotRed/Core/Input.h"
#include "AssetEditorPanel.h"

namespace NR
{
	static int sColumnCount = 2;

	ContentBrowserPanel::ContentBrowserPanel()
	{
		sInstance = this;

		AssetManager::SetAssetChangeCallback(NR_BIND_EVENT_FN(ContentBrowserPanel::FileSystemChanged));

		mFileTex = AssetManager::GetAsset<Texture2D>("Assets/Editor/file.png");
		mFolderIcon = AssetManager::GetAsset<Texture2D>("Assets/Editor/folder.png");
		mRefreshIcon = Texture2D::Create("Assets/Editor/refresh.png");
		mAssetIconMap[""] = AssetManager::GetAsset<Texture2D>("Assets/Editor/folder.png");
		mAssetIconMap["fbx"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/fbx.png");
		mAssetIconMap["obj"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/obj.png");
		mAssetIconMap["wav"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/wav.png");
		mAssetIconMap["cs"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/csc.png");
		mAssetIconMap["png"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/png.png");
		mAssetIconMap["nrsc"] = AssetManager::GetAsset<Texture2D>("Assets/Editor/notred.png");

		mBackbtnTex = AssetManager::GetAsset<Texture2D>("Assets/Editor/back.png");
		mFwrdbtnTex = AssetManager::GetAsset<Texture2D>("Assets/Editor/fwrd.png");

		AssetHandle baseDirectoryHandle = ProcessDirectory("Assets", nullptr);
		mBaseDirectory = mDirectories[baseDirectoryHandle];
		ChangeDirectory(mBaseDirectory);

		memset(mSearchBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
	}

	AssetHandle ContentBrowserPanel::ProcessDirectory(const std::string& directoryPath, const Ref<DirectoryInfo>& parent)
	{
		std::string fixedFilePath = directoryPath;
		std::replace(fixedFilePath.begin(), fixedFilePath.end(), '\\', '/');

		const auto& directory = GetDirectory(fixedFilePath);
		if (directory)
		{
			return directory->Handle;
		}

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
				AssetHandle subdirHandle = ProcessDirectory(entry.path().string(), directoryInfo);
				directoryInfo->SubDirectories[subdirHandle] = mDirectories[subdirHandle];
				continue;
			}

			auto metadata = AssetManager::GetMetadata(entry.path().string());
			if (!metadata.IsValid())
			{
				AssetType type = AssetManager::GetAssetTypeForFileType(Utils::GetExtension(entry.path().string()));
				if (type == AssetType::None)
				{
					continue;
				}
				metadata.Handle = AssetManager::ImportAsset(entry.path().string());
			}

			if (!metadata.IsValid())
			{
				continue;
			}
			directoryInfo->Assets.push_back(metadata.Handle);
		}

		mDirectories[directoryInfo->Handle] = directoryInfo;
		return directoryInfo->Handle;
	}

	void ContentBrowserPanel::ChangeDirectory(Ref<DirectoryInfo>& directory)
	{
		mUpdateNavigationPath = true;
		mCurrentItems.Items.clear();

		for (auto& [subdirHandle, subdir] : directory->SubDirectories)
		{
			mCurrentItems.Items.push_back(Ref<ContentBrowserDirectory>::Create(subdir));
		}

		std::vector<AssetHandle> invalidAssets;
		for (auto assetHandle : directory->Assets)
		{
			AssetMetadata metadata = AssetManager::GetMetadata(assetHandle);
			if (!metadata.IsValid())
				invalidAssets.emplace_back(metadata.Handle);
			else
			{
				mCurrentItems.Items.push_back(Ref<ContentBrowserAsset>::Create(metadata, mAssetIconMap.find(metadata.Extension) != mAssetIconMap.end() ? mAssetIconMap[metadata.Extension] : mFileTex));
			}
		}

		for (auto invalidHandle : invalidAssets)
		{
			directory->Assets.erase(std::remove(directory->Assets.begin(), directory->Assets.end(), invalidHandle), directory->Assets.end());
		}

		SortItemList();

		mPreviousDirectory = directory;
		mCurrentDirectory = directory;
	}

	void ContentBrowserPanel::ImGuiRender()
	{
		ImGui::Begin("Content Browser", NULL, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
		{
			UI::BeginPropertyGrid();
			ImGui::SetColumnOffset(1, 300.0f);

			ImGui::BeginChild("##folders_common");
			{
				if (ImGui::CollapsingHeader("Content", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
				{
					for (auto& [handle, directory] : mBaseDirectory->SubDirectories)
					{
						RenderDirectoryHeirarchy(directory);
					}
				}
			}
			ImGui::EndChild();

			ImGui::NextColumn();

			ImGui::BeginChild("##directory_structure", ImVec2(0, ImGui::GetWindowHeight() - 65));
			{
				RenderTopBar();

				ImGui::Separator();

				ImGui::BeginChild("Scrolling");
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.35f));

					if (ImGui::BeginPopupContextWindow(0, 1))
					{
						if (ImGui::BeginMenu("New"))
						{
							if (ImGui::MenuItem("Folder"))
							{
								FileSystem::CreateFolder(mCurrentDirectory->FilePath + "/New Folder");
							}

							if (ImGui::MenuItem("Physics Material"))
							{
								CreateAsset<PhysicsMaterial>("New Physics Material.hpm", 0.6f, 0.6f, 0.0f);
							}

							if (ImGui::MenuItem("Sound Config"))
							{
								CreateAsset<Audio::SoundConfig>("New Sound Config.hsoundc");
							}

							ImGui::EndMenu();
						}

						if (ImGui::MenuItem("Import"))
						{
							std::string filepath = Application::Get().OpenFile();
							if (!filepath.empty())
							{
								FileSystem::MoveFile(filepath, mCurrentDirectory->FilePath);
							}
						}

						if (ImGui::MenuItem("Refresh"))
						{
							Refresh();
						}

						ImGui::EndPopup();
					}

					ImGui::Columns(sColumnCount, nullptr, false);
					
					RenderItems();

					if (ImGui::IsWindowFocused() && !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
					{
						UpdateInput();
					}

					ImGui::PopStyleColor(2);					
					
					RenderDeleteDialogue();
				}
				ImGui::EndChild();
			}
			ImGui::EndChild();

			RenderBottomBar();

			UI::EndPropertyGrid();
		}
		ImGui::End();
	}

	Ref<DirectoryInfo> ContentBrowserPanel::GetDirectory(const std::string& filepath) const
	{
		for (const auto& [handle, directory] : mDirectories)
		{
			if (directory->FilePath == filepath)
			{
				return directory;
			}
		}

		return nullptr;
	}

	namespace UI
	{
		static bool TreeNode(const std::string& id, const std::string& label, ImGuiTreeNodeFlags flags = 0)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			if (window->SkipItems)
			{
				return false;
			}

			return ImGui::TreeNodeBehavior(window->GetID(id.c_str()), flags, label.c_str(), NULL);
		}
	}

	void ContentBrowserPanel::RenderDirectoryHeirarchy(Ref<DirectoryInfo>& directory)
	{
		std::string id = directory->Name + "_TreeNode";
		bool previousState = ImGui::TreeNodeBehaviorIsOpen(ImGui::GetID(id.c_str()));
		bool open = UI::TreeNode(id, directory->Name, ImGuiTreeNodeFlags_SpanAvailWidth);

		if (open)
		{
			for (auto& [handle, child] : directory->SubDirectories)
			{
				RenderDirectoryHeirarchy(child);
			}
		}

		UpdateDropArea(directory);

		if (open != previousState && directory->Handle != mCurrentDirectory->Handle)
		{
			if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.01f))
				ChangeDirectory(directory);
		}

		if (open)
		{
			ImGui::TreePop();
		}
	}

	void ContentBrowserPanel::RenderTopBar()
	{
		ImGui::BeginChild("##top_bar", ImVec2(0, 30));
		{
			if (UI::ImageButton(mBackbtnTex->GetImage(), ImVec2(25, 25)))
			{
				if (mCurrentDirectory->Handle == mBaseDirectory->Handle)
				{
					return;
				}

				mNextDirectory = mCurrentDirectory;
				mPreviousDirectory = mCurrentDirectory->Parent;
				ChangeDirectory(mPreviousDirectory);
			}

			ImGui::SameLine();

			if (UI::ImageButton(mFwrdbtnTex->GetImage(), ImVec2(25, 25)))
			{
				ChangeDirectory(mNextDirectory);
			}

			ImGui::SameLine();

			if (UI::ImageButton(mRefreshIcon, ImVec2(25, 25)))
			{
				Refresh();
			}

			ImGui::SameLine();

			{
				ImGui::PushItemWidth(200);
				if (ImGui::InputTextWithHint("##", "Search...", mSearchBuffer, MAX_INPUT_BUFFER_LENGTH))
				{
					if (strlen(mSearchBuffer) == 0)
					{
						ChangeDirectory(mCurrentDirectory);
					}
					else
					{
						mCurrentItems = Search(mSearchBuffer, mCurrentDirectory);
						SortItemList();
					}
				}

				ImGui::PopItemWidth();
			}

			ImGui::SameLine();

			if (mUpdateNavigationPath)
			{
				mBreadCrumbData.clear();

				Ref<DirectoryInfo> current = mCurrentDirectory;
				while (current && current->Handle != 0)
				{
					mBreadCrumbData.push_back(current);
					current = current->Parent;
				}

				std::reverse(mBreadCrumbData.begin(), mBreadCrumbData.end());

				mUpdateNavigationPath = false;
			}

			for (auto& directory : mBreadCrumbData)
			{
				if (directory->Name != "assets")
				{
					ImGui::Text("/");
				}

				ImGui::SameLine();

				ImVec2 textSize = ImGui::CalcTextSize(directory->Name.c_str());
				if (ImGui::Selectable(directory->Name.c_str(), false, 0, ImVec2(textSize.x, 22)))
				{
					ChangeDirectory(directory);
				}

				UpdateDropArea(directory);

				ImGui::SameLine();
			}

		}

		ImGui::EndChild();
	}

	static std::mutex sLockMutex;
	void ContentBrowserPanel::RenderItems()
	{
		mIsAnyItemHovered = false;

		bool openDeleteDialogue = false;

		std::lock_guard<std::mutex> lock(sLockMutex);
		for (auto& item : mCurrentItems)
		{
			item->RenderBegin();
			CBItemActionResult result = item->Render();

			if (result.IsSet(ContentBrowserAction::ClearSelections))
			{
				ClearSelections();
			}

			if (result.IsSet(ContentBrowserAction::Selected) && !mSelectionStack.IsSelected(item->GetID()))
			{
				mSelectionStack.Select(item->GetID());
				item->SetSelected(true);
			}

			if (result.IsSet(ContentBrowserAction::DeSelected) && mSelectionStack.IsSelected(item->GetID()))
			{
				mSelectionStack.Deselect(item->GetID());
				item->SetSelected(false);
			}

			if (result.IsSet(ContentBrowserAction::SelectToHere) && mSelectionStack.SelectionCount() == 2)
			{
				size_t firstIndex = mCurrentItems.FindItem(mSelectionStack[0]);
				size_t lastIndex = mCurrentItems.FindItem(item->GetID());

				if (firstIndex > lastIndex)
				{
					size_t temp = firstIndex;
					firstIndex = lastIndex;
					lastIndex = temp;
				}

				for (size_t i = firstIndex; i <= lastIndex; ++i)
				{
					auto toSelect = mCurrentItems[i];
					item->SetSelected(true);
					mSelectionStack.Select(item->GetID());
				}
			}

			if (result.IsSet(ContentBrowserAction::OpenDeleteDialogue))
			{
				openDeleteDialogue = true;
			}
			if (result.IsSet(ContentBrowserAction::Hovered))
			{
				mIsAnyItemHovered = true;
			}

			item->RenderEnd();

			if (result.IsSet(ContentBrowserAction::Renamed))
			{
				SortItemList();
				break;
			}

			if (result.IsSet(ContentBrowserAction::NavigateToThis) && item->GetType() == ContentBrowserItem::ItemType::Directory)
			{
				ChangeDirectory(item.As<ContentBrowserDirectory>()->GetDirectoryInfo());
				break;
			}

			if (result.IsSet(ContentBrowserAction::Refresh))
			{
				ChangeDirectory(mCurrentDirectory);
				break;
			}
		}

		if (openDeleteDialogue)
		{
			ImGui::OpenPopup("Delete");
			openDeleteDialogue = false;
		}
	}

	void ContentBrowserPanel::RenderBottomBar()
	{
		ImGui::BeginChild("##panel_controls", ImVec2(ImGui::GetColumnWidth() - 12, 30), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		{
			ImGui::Separator();

			ImGui::Columns(4, 0, false);
			if (mSelectionStack.SelectionCount() == 1)
			{
				const AssetMetadata& asset = AssetManager::GetMetadata(mSelectionStack[0]);

				if (asset.IsValid())
				{
					ImGui::Text(asset.FilePath.c_str());
				}
				else if (mDirectories.find(mSelectionStack[0]) != mDirectories.end())
				{
					ImGui::Text(mDirectories[mSelectionStack[0]]->FilePath.c_str());
				}
			}
			else if (mSelectionStack.SelectionCount() > 1)
			{
				ImGui::Text("%d items selected", mSelectionStack.SelectionCount());
			}

			ImGui::NextColumn();
			ImGui::NextColumn();
			ImGui::NextColumn();
			ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
			ImGui::SliderInt("##column_count", &sColumnCount, 2, 15);
		}

		ImGui::EndChild();
	}

	void ContentBrowserPanel::Refresh()
	{
		for (auto entry : std::filesystem::directory_iterator(mCurrentDirectory->FilePath))
		{
			if (!entry.is_directory())
			{
				const auto& assetInfo = AssetManager::GetMetadata(entry.path().string());
				if (!assetInfo.IsValid())
				{
					AssetHandle handle = AssetManager::ImportAsset(entry.path().string());
					mCurrentDirectory->Assets.push_back(handle);
				}
			}
			else
			{
				std::string fixedFilePath = entry.path().string();
				std::replace(fixedFilePath.begin(), fixedFilePath.end(), '\\', '/');

				const auto& directory = GetDirectory(fixedFilePath);
				if (!directory)
				{
					AssetHandle directoryHandle = ProcessDirectory(entry.path().string(), mCurrentDirectory);
					mCurrentDirectory->SubDirectories[directoryHandle] = mDirectories[directoryHandle];
				}
			}
		}

		ChangeDirectory(mCurrentDirectory);
	}

	void ContentBrowserPanel::UpdateInput()
	{
		if ((!mIsAnyItemHovered && ImGui::IsAnyMouseDown()) || Input::IsKeyPressed(KeyCode::Escape))
		{
			ClearSelections();
		}

		if (Input::IsKeyPressed(KeyCode::Delete))
		{
			ImGui::OpenPopup("Delete");
		}

		if (Input::IsKeyPressed(KeyCode::F5))
		{
			Refresh();
		}
	}

	void ContentBrowserPanel::ClearSelections()
	{
		for (auto selectedHandle : mSelectionStack)
		{
			for (auto& item : mCurrentItems)
			{
				if (item->GetID() == selectedHandle)
				{
					item->SetSelected(false);
				}
			}
		}

		mSelectionStack.Clear();
	}

	void ContentBrowserPanel::RenderDeleteDialogue()
	{
		if (ImGui::BeginPopupModal("Delete", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Are you sure you want to delete %d items?", mSelectionStack.SelectionCount());

			float columnWidth = ImGui::GetContentRegionAvail().x / 4;

			ImGui::Columns(4, 0, false);
			ImGui::SetColumnWidth(0, columnWidth);
			ImGui::SetColumnWidth(1, columnWidth);
			ImGui::SetColumnWidth(2, columnWidth);
			ImGui::SetColumnWidth(3, columnWidth);
			ImGui::NextColumn();

			if (ImGui::Button("Yes", ImVec2(columnWidth, 0)))
			{
				for (AssetHandle handle : mSelectionStack)
				{
					size_t index = mCurrentItems.FindItem(handle);
					if (index == ContentBrowserItemList::InvalidItem)
					{
						continue;
					}
					mCurrentItems[index]->Delete();
					mCurrentItems.erase(handle);
				}
				for (AssetHandle handle : mSelectionStack)
				{
					if (mDirectories.find(handle) != mDirectories.end())
					{
						RemoveDirectory(mDirectories[handle]);
					}
				}

				mSelectionStack.Clear();

				ChangeDirectory(mCurrentDirectory);

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
	}

	void ContentBrowserPanel::RemoveDirectory(Ref<DirectoryInfo>& directory)
	{
		if (directory->Parent != 0)
		{
			auto& childList = directory->Parent->SubDirectories;
			childList.erase(childList.find(directory->Handle));
		}

		for (auto& [handle, subdir] : directory->SubDirectories)
		{
			RemoveDirectory(subdir);
		}

		directory->SubDirectories.clear();
		directory->Assets.clear();

		mDirectories.erase(mDirectories.find(directory->Handle));
	}

	void ContentBrowserPanel::UpdateDropArea(const Ref<DirectoryInfo>& target)
	{
		if (target->Handle != mCurrentDirectory->Handle && ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("asset_payload");

			if (payload)
			{
				uint32_t count = payload->DataSize / sizeof(AssetHandle);

				for (uint32_t i = 0; i < count; ++i)
				{
					AssetHandle assetHandle = *(((AssetHandle*)payload->Data) + i);
					size_t index = mCurrentItems.FindItem(assetHandle);
					if (index != ContentBrowserItemList::InvalidItem)
					{
						mCurrentItems[index]->Move(target->FilePath);
						mCurrentItems.erase(assetHandle);
					}
				}
			}

			ImGui::EndDragDropTarget();
		}
	}

	void ContentBrowserPanel::SortItemList()
	{
		std::sort(mCurrentItems.begin(), mCurrentItems.end(), [](const Ref<ContentBrowserItem>& item1, const Ref<ContentBrowserItem>& item2)
			{
				if (item1->GetType() == item2->GetType())
				{
					return Utils::ToLower(item1->GetName()) < Utils::ToLower(item2->GetName());
				}

				return (uint16_t)item1->GetType() < (uint16_t)item2->GetType();
			});
	}

	ContentBrowserItemList ContentBrowserPanel::Search(const std::string& query, const Ref<DirectoryInfo>& directoryInfo)
	{
		ContentBrowserItemList results;
		std::string queryLowerCase = Utils::ToLower(query);

		for (auto& [handle, subdir] : directoryInfo->SubDirectories)
		{
			if (subdir->Name.find(queryLowerCase) != std::string::npos)
			{
				results.Items.push_back(Ref<ContentBrowserDirectory>::Create(subdir));
			}

			ContentBrowserItemList list = Search(query, subdir);
			results.Items.insert(results.Items.end(), list.Items.begin(), list.Items.end());
		}

		for (auto& assetHandle : directoryInfo->Assets)
		{
			auto& asset = AssetManager::GetMetadata(assetHandle);
			std::string filename = Utils::ToLower(asset.FileName);

			if (filename.find(queryLowerCase) != std::string::npos)
			{
				results.Items.push_back(Ref<ContentBrowserAsset>::Create(asset, mAssetIconMap.find(asset.Extension) != mAssetIconMap.end() ? mAssetIconMap[asset.Extension] : mFileTex));
			}

			if (queryLowerCase[0] != '.')
			{
				continue;
			}

			if (asset.Extension.find(std::string(&queryLowerCase[1])) != std::string::npos)
			{
				results.Items.push_back(Ref<ContentBrowserAsset>::Create(asset, mAssetIconMap.find(asset.Extension) != mAssetIconMap.end() ? mAssetIconMap[asset.Extension] : mFileTex));
			}
		}

		return results;
	}

	void ContentBrowserPanel::FileSystemChanged(FileSystemChangedEvent event)
	{
		NR_CORE_ERROR("OnFileSystemChanged");
		std::lock_guard<std::mutex> lock(sLockMutex);
		switch (event.Action)
		{
		case FileSystemAction::Added:
		{
			if (event.IsDirectory)
			{
				DirectoryAdded(event);
			}
			else
			{
				AssetAdded(event);
			}
			break;
		}
		case FileSystemAction::Delete:
		{
			if (event.IsDirectory)
			{
				DirectoryDeleted(event);
			}
			else
			{
				AssetDeleted(event);
			}
			break;
		}
		case FileSystemAction::Modified:
		{
			break;
		}
		case FileSystemAction::Rename:
		{
			if (event.IsDirectory)
			{
				DirectoryRenamed(event);
			}
			else
			{
				AssetRenamed(event);
			}
			break;
		}
		}
	}

	void ContentBrowserPanel::AssetAdded(FileSystemChangedEvent event)
	{
		const auto& assetMetadata = AssetManager::GetMetadata(event.FilePath);
		if (!assetMetadata.IsValid())
		{
			return;
		}

		std::filesystem::path path = assetMetadata.FilePath;
		auto directory = GetDirectory(path.parent_path().string());

		if (!directory)
		{
			NR_CORE_ASSERT(false, "How did this even happen?");
		}
		directory->Assets.push_back(assetMetadata.Handle);

		ChangeDirectory(mCurrentDirectory);
	}

	void ContentBrowserPanel::DirectoryAdded(FileSystemChangedEvent event)
	{
		std::filesystem::path path = event.FilePath;
		auto parentDirectory = GetDirectory(path.parent_path().string());
		if (!parentDirectory)
		{
			NR_CORE_ASSERT(false, "How did this even happen?");
		}

		AssetHandle directoryHandle = ProcessDirectory(event.FilePath, parentDirectory);
		if (directoryHandle == 0)
		{
			return;
		}

		parentDirectory->SubDirectories[directoryHandle] = mDirectories[directoryHandle];

		ChangeDirectory(mCurrentDirectory);

		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
		{
			size_t directoryIndex = mCurrentItems.FindItem(directoryHandle);
			if (directoryIndex == ContentBrowserItemList::InvalidItem)
			{
				return;
			}

			auto& item = mCurrentItems[directoryIndex];
			mSelectionStack.Select(directoryHandle);

			item->SetSelected(true);
			item->StartRenaming();
		}
	}

	void ContentBrowserPanel::AssetDeleted(FileSystemChangedEvent event)
	{
		AssetMetadata metadata;
		Ref<DirectoryInfo> directory;
		for (const auto& item : mCurrentItems.Items)
		{
			if (item->GetType() == ContentBrowserItem::ItemType::Asset)
			{
				const auto& assetInfo = item.As<ContentBrowserAsset>()->GetAssetInfo();

				if (assetInfo.FilePath == event.FilePath)
				{
					metadata = assetInfo;

					std::string parentDirectory = std::filesystem::path(assetInfo.FilePath).parent_path().string();
					directory = GetDirectory(parentDirectory);
					break;
				}
			}
		}

		AssetDeleted(metadata, directory);
		ChangeDirectory(mCurrentDirectory);
	}

	void ContentBrowserPanel::AssetDeleted(AssetMetadata metadata, Ref<DirectoryInfo> directory)
	{
		if (!metadata.IsValid() || !directory)
			return;

		if (AssetManager::IsAssetHandleValid(metadata.Handle))
		{
			AssetManager::AssetDeleted(metadata.Handle);
		}

		directory->Assets.erase(std::remove(directory->Assets.begin(), directory->Assets.end(), metadata.Handle), directory->Assets.end());
		mCurrentItems.erase(metadata.Handle);
	}

	void ContentBrowserPanel::AssetRenamed(FileSystemChangedEvent event)
	{
		if (!event.WasTracking)
		{
			auto& assetInfo = AssetManager::GetMetadata(event.FilePath);
			std::filesystem::path path = event.FilePath;
			auto directory = GetDirectory(path.parent_path().string());
			directory->Assets.push_back(assetInfo.Handle);
			ChangeDirectory(mCurrentDirectory);
		}
		else
		{
			auto& assetInfo = AssetManager::GetMetadata(event.FilePath);
			if (!assetInfo.IsValid())
			{
				return;
			}

			size_t index = mCurrentItems.FindItem(assetInfo.Handle);
			if (index != ContentBrowserItemList::InvalidItem)
			{
				mCurrentItems[index]->Rename(event.NewName, true);
			}
		}
	}

	void ContentBrowserPanel::DirectoryDeleted(FileSystemChangedEvent event)
	{
		DirectoryDeleted(GetDirectory(event.FilePath));
	}

	void ContentBrowserPanel::DirectoryDeleted(Ref<DirectoryInfo> directory, uint32_t depth)
	{
		if (!directory)
		{
			return;
		}

		for (auto asset : directory->Assets)
		{
			AssetDeleted(AssetManager::GetMetadata(asset), directory);
		}

		for (auto [subdirHandle, subdir] : directory->SubDirectories)
		{
			DirectoryDeleted(subdir, depth + 1);
		}

		directory->Assets.clear();
		directory->SubDirectories.clear();

		if (depth == 0 && directory->Parent)
		{
			directory->Parent->SubDirectories.erase(directory->Handle);
		}

		mCurrentItems.erase(directory->Handle);
		mDirectories.erase(directory->Handle);

		ChangeDirectory(mCurrentDirectory);
	}

	void ContentBrowserPanel::DirectoryRenamed(FileSystemChangedEvent event)
	{
		std::filesystem::path path = event.FilePath;
		auto directory = GetDirectory(path.parent_path().string() + "/" + event.OldName);

		size_t itemIndex = mCurrentItems.FindItem(directory->Handle);
		if (itemIndex != ContentBrowserItemList::InvalidItem)
		{
			mCurrentItems[itemIndex]->Rename(event.NewName, true);
		}
		else
		{
			directory->Name = event.NewName;
			UpdateDirectoryPath(directory, path.parent_path().string());
		}

		ChangeDirectory(mCurrentDirectory);
	}

	void ContentBrowserPanel::UpdateDirectoryPath(Ref<DirectoryInfo>& directoryInfo, const std::string& newParentPath)
	{
		directoryInfo->FilePath = newParentPath + "/" + directoryInfo->Name;

		for (auto& assetHandle : directoryInfo->Assets)
		{
			auto& metadata = AssetManager::GetMetadata(assetHandle);
			metadata.FilePath = directoryInfo->FilePath + "/" + metadata.FileName + "." + metadata.Extension;
		}

		for (auto& [handle, subdirectory] : directoryInfo->SubDirectories)
		{
			UpdateDirectoryPath(subdirectory, directoryInfo->FilePath);
		}
	}

	ContentBrowserPanel* ContentBrowserPanel::sInstance;
}