#include "nrpch.h"
#include "ContentBrowserItem.h"

#include <filesystem>

#include "imgui_internal.h"

#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Util/FileSystem.h"
#include "NotRed/ImGui/ImGui.h"
#include "NotRed/Editor/ContentBrowserPanel.h"
#include "NotRed/Editor/AssetEditorPanel.h"

namespace NR
{
	static char sRenameBuffer[MAX_INPUT_BUFFER_LENGTH];

	ContentBrowserItem::ContentBrowserItem(ItemType type, AssetHandle id, const std::string& name, const Ref<Texture2D>& icon)
		: mType(type), mID(id), mName(name), mIcon(icon)
	{
	}

	void ContentBrowserItem::RenderBegin()
	{
		ImGui::PushID(&mID);
		ImGui::BeginGroup();
	}

	CBItemActionResult ContentBrowserItem::Render()
	{
		CBItemActionResult result;

		if (mIsSelected)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 0.75f));
		}

		float buttonWidth = ImGui::GetColumnWidth() - 15.0f;
		UI::ImageButton(mName.c_str(), mIcon, { buttonWidth, buttonWidth });
		if (mIsSelected)
		{
			ImGui::PopStyleColor();
		}

		UpdateDrop(result);
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			const auto& selectionStack = ContentBrowserPanel::Get().GetSelectionStack();
			auto& currentItems = ContentBrowserPanel::Get().GetCurrentItems();
			if (selectionStack.SelectionCount() > 0)
			{
				for (const auto& selectedItemHandles : selectionStack)
				{
					size_t index = currentItems.FindItem(selectedItemHandles);
					if (index == ContentBrowserItemList::InvalidItem)
					{
						continue;
					}
					const auto& item = currentItems[index];
					UI::Image(item->GetIcon(), ImVec2(20, 20));
					ImGui::SameLine();
					const auto& name = item->GetName();
					ImGui::TextUnformatted(name.c_str());
				}
				ImGui::SetDragDropPayload("asset_payload", selectionStack.SelectionData(), sizeof(AssetHandle) * selectionStack.SelectionCount());
			}
			result.Modify(ContentBrowserAction::Selected, true);
			ImGui::EndDragDropSource();
		}

		if (ImGui::IsItemHovered())
		{
			result.Modify(ContentBrowserAction::Hovered, true);
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				Activate(result);
			}
			else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				if (mIsSelected)
				{
					result.Modify(ContentBrowserAction::DeSelected, true);
				}
				else
				{
					result.Modify(ContentBrowserAction::Selected, true);
				}
				if (!Input::IsKeyPressed(KeyCode::LeftControl) && !Input::IsKeyPressed(KeyCode::LeftShift))
				{
					result.Modify(ContentBrowserAction::ClearSelections, true);
				}
				if (Input::IsKeyPressed(KeyCode::LeftShift))
				{
					result.Modify(ContentBrowserAction::SelectToHere, true);
				}
			}
		}

		if (ImGui::BeginPopupContextItem("CBItemContextMenu"))
		{
			result.Modify(ContentBrowserAction::Selected, true);
			ContextMenuOpen(result);
			ImGui::EndPopup();
		}

		if (!mIsRenaming)
		{
			ImGui::TextWrapped(mName.c_str());
			if (Input::IsKeyPressed(KeyCode::F2) && mIsSelected)
			{
				StartRenaming();
			}
		}
		else
		{
			ImGui::SetKeyboardFocusHere();
			if (ImGui::InputText("##rename", sRenameBuffer, MAX_INPUT_BUFFER_LENGTH, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				Rename(sRenameBuffer);
				mIsRenaming = false;
				result.Modify(ContentBrowserAction::Renamed, true);
			}
		}

		return result;
	}

	void ContentBrowserItem::StartRenaming()
	{
		if (mIsRenaming)
		{
			return;
		}
		memset(sRenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
		memcpy(sRenameBuffer, mName.c_str(), mName.size());
		mIsRenaming = true;
	}

	void ContentBrowserItem::SetSelected(bool value)
	{
		mIsSelected = value;
		if (!mIsSelected)
		{
			mIsRenaming = false;
			memset(sRenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
		}
	}

	void ContentBrowserItem::Rename(const std::string& newName, bool fromCallback)
	{
		Renamed(newName, fromCallback);
		mName = newName;
	}

	void ContentBrowserItem::ContextMenuOpen(CBItemActionResult& actionResult)
	{
		if (ImGui::MenuItem("Rename"))
		{
			StartRenaming();
		}
		if (ImGui::MenuItem("Delete"))
		{
			actionResult.Modify(ContentBrowserAction::OpenDeleteDialogue, true);
		}
		RenderCustomContextItems();
	}

	void ContentBrowserItem::RenderEnd()
	{
		ImGui::EndGroup();
		ImGui::PopID();
		ImGui::NextColumn();
	}

	ContentBrowserDirectory::ContentBrowserDirectory(const Ref<DirectoryInfo>& directoryInfo)
		: ContentBrowserItem(ContentBrowserItem::ItemType::Directory, directoryInfo->Handle, directoryInfo->Name, AssetManager::GetAsset<Texture2D>("assets/editor/folder.png")), mDirectoryInfo(directoryInfo)
	{
	}

	void ContentBrowserDirectory::Activate(CBItemActionResult& actionResult)
	{
		actionResult.Modify(ContentBrowserAction::NavigateToThis, true);
	}

	void ContentBrowserDirectory::Renamed(const std::string& newName, bool fromCallback)
	{
		std::filesystem::path path = mDirectoryInfo->FilePath;
		std::string newFilePath = path.parent_path().string() + "/" + newName;
		if (!fromCallback)
		{
			if (FileSystem::Exists(newFilePath))
			{
				NR_CORE_ERROR("A directory with that name already exists!");
				return;
			}
			FileSystem::Rename(mDirectoryInfo->FilePath, newName);
		}
		mDirectoryInfo->Name = newName;
		UpdateDirectoryPath(mDirectoryInfo, path.parent_path().string());
	}

	void ContentBrowserDirectory::UpdateDrop(CBItemActionResult& actionResult)
	{
		if (IsSelected())
		{
			return;
		}

		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("asset_payload");
			if (payload)
			{
				auto& currentItems = ContentBrowserPanel::Get().GetCurrentItems();
				uint32_t count = payload->DataSize / sizeof(AssetHandle);
				for (uint32_t i = 0; i < count; ++i)
				{
					AssetHandle assetHandle = *(((AssetHandle*)payload->Data) + i);
					size_t index = currentItems.FindItem(assetHandle);
					if (index != ContentBrowserItemList::InvalidItem)
					{
						if (currentItems[index]->Move(mDirectoryInfo->FilePath))
						{
							actionResult.Modify(ContentBrowserAction::Refresh, true);
							currentItems.erase(assetHandle);
						}
					}
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	void ContentBrowserDirectory::Delete()
	{
		FileSystem::DeleteFile(mDirectoryInfo->FilePath);
	}

	bool ContentBrowserDirectory::Move(const std::string& destination)
	{
		bool wasMoved = FileSystem::MoveFile(mDirectoryInfo->FilePath, destination);
		if (!wasMoved)
		{
			return false;
		}

		UpdateDirectoryPath(mDirectoryInfo, destination);

		auto& parentSubdirs = mDirectoryInfo->Parent->SubDirectories;
		parentSubdirs.erase(parentSubdirs.find(mDirectoryInfo->Handle));

		auto newParent = ContentBrowserPanel::Get().GetDirectory(destination);
		newParent->SubDirectories[mDirectoryInfo->Handle] = mDirectoryInfo;
		mDirectoryInfo->Parent = newParent;

		return true;
	}
	void ContentBrowserDirectory::UpdateDirectoryPath(Ref<DirectoryInfo> directoryInfo, const std::string& newParentPath)
	{
		directoryInfo->FilePath = newParentPath + "/" + directoryInfo->Name;
		for (auto assetHandle : directoryInfo->Assets)
		{
			auto metadata = AssetManager::GetMetadata(assetHandle);
			{
				metadata.FilePath = directoryInfo->FilePath + "/" + metadata.FileName + "." + metadata.Extension;
			}
		}
		for (auto [handle, subdirectory] : directoryInfo->SubDirectories)
		{
			UpdateDirectoryPath(subdirectory, directoryInfo->FilePath);
		}
	}

	ContentBrowserAsset::ContentBrowserAsset(const AssetMetadata& assetInfo, const Ref<Texture2D>& icon)
		: ContentBrowserItem(ContentBrowserItem::ItemType::Asset, assetInfo.Handle, assetInfo.FileName, icon), mAssetInfo(assetInfo)
	{
	}

	void ContentBrowserAsset::Delete()
	{
		bool deleted = FileSystem::DeleteFile(mAssetInfo.FilePath);
		if (!deleted)
		{
			NR_CORE_ERROR("Couldn't delete {0}", mAssetInfo.FilePath);
			return;
		}
		std::filesystem::path currentPath = mAssetInfo.FilePath;
		auto currentDirectory = ContentBrowserPanel::Get().GetDirectory(currentPath.parent_path().string());
		currentDirectory->Assets.erase(std::remove(currentDirectory->Assets.begin(), currentDirectory->Assets.end(), mAssetInfo.Handle), currentDirectory->Assets.end());
		
		AssetManager::AssetDeleted(mAssetInfo.Handle);
	}

	bool ContentBrowserAsset::Move(const std::string& destination)
	{
		bool wasMoved = FileSystem::MoveFile(mAssetInfo.FilePath, destination);
		if (!wasMoved)
		{
			NR_CORE_ERROR("Couldn't move {0} to {1}", mAssetInfo.FilePath, destination);
			return false;
		}
		std::filesystem::path currentPath = mAssetInfo.FilePath;
		auto currentDirectory = ContentBrowserPanel::Get().GetDirectory(currentPath.parent_path().string());
		currentDirectory->Assets.erase(std::remove(currentDirectory->Assets.begin(), currentDirectory->Assets.end(), mAssetInfo.Handle), currentDirectory->Assets.end());
		AssetManager::AssetMoved(mAssetInfo.Handle, destination);
		return true;
	}

	void ContentBrowserAsset::Activate(CBItemActionResult& actionResult)
	{
		if (mAssetInfo.Type == AssetType::Scene)
		{
		}
		else
		{
			AssetEditorPanel::OpenEditor(AssetManager::GetAsset<Asset>(mAssetInfo.Handle));
		}
	}

	void ContentBrowserAsset::Renamed(const std::string& newName, bool fromCallback)
	{
		std::filesystem::path path = mAssetInfo.FilePath;
		std::string newFilePath = path.parent_path().string() + "/" + newName + "." + mAssetInfo.Extension;
		if (!fromCallback)
		{
			if (FileSystem::Exists(newFilePath))
			{
				NR_CORE_ERROR("A file with that name already exists!");
				return;
			}
			FileSystem::Rename(mAssetInfo.FilePath, newName);
			AssetManager::AssetRenamed(mAssetInfo.Handle, newFilePath);
		}
		mAssetInfo.FilePath = newFilePath;
		mAssetInfo.FileName = Utils::RemoveExtension(Utils::GetFilename(newFilePath));
	}
}