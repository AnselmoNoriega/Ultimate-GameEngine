#include "nrpch.h"
#include "ContentBrowserItem.h"

#include "Panels/ContentBrowserPanel.h"

#include "imgui_internal.h"

#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Util/FileSystem.h"
#include "NotRed/ImGui/ImGui.h"
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

	CBItemActionResult ContentBrowserItem::Render(float thumbnailSize, bool displayAssetType)
	{
		CBItemActionResult result;

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
		
		const float edgeOffset = 4.0f;
		const float textLineHeight = ImGui::GetTextLineHeightWithSpacing() * 2.0f + edgeOffset * 2.0f;
		const float infoPanelHeight = std::max(displayAssetType ? thumbnailSize * 0.5f : textLineHeight, textLineHeight);

		const ImVec2 topLeft = ImGui::GetCursorScreenPos();
		const ImVec2 thumbBottomRight = { topLeft.x + thumbnailSize, topLeft.y + thumbnailSize };
		const ImVec2 infoTopLeft = { topLeft.x,				 topLeft.y + thumbnailSize };
		const ImVec2 bottomRight = { topLeft.x + thumbnailSize, topLeft.y + thumbnailSize + infoPanelHeight };

		auto drawShadow = [](const ImVec2& topLeft, const ImVec2& bottomRight, bool directory)
			{
				auto* drawList = ImGui::GetWindowDrawList();
				const ImRect itemRect = UI::RectOffset(ImRect(topLeft, bottomRight), 1.0f, 1.0f);
				drawList->AddRect(itemRect.Min, itemRect.Max, Colors::Theme::propertyField, 6.0f, directory ? 0 : ImDrawFlags_RoundCornersBottom, 2.0f);
			};

		// Fill background
		//----------------
		if (mType != ItemType::Directory)
		{
			const ImRect expandedArea = UI::RectExpanded(ImRect(topLeft, bottomRight), 2.0f, 2.0f);
			ImGui::PushClipRect(expandedArea.Min, expandedArea.Max, false);
			auto* drawList = ImGui::GetWindowDrawList();

			// Draw shadow
			drawShadow(topLeft, bottomRight, false);

			// Draw background
			drawList->AddRectFilled(topLeft, thumbBottomRight, Colors::Theme::backgroundDark);
			drawList->AddRectFilled(infoTopLeft, bottomRight, Colors::Theme::groupHeader, 6.0f, ImDrawFlags_RoundCornersBottom);
			ImGui::PopClipRect();
		}
		else if (ImGui::ItemHoverable(ImRect(topLeft, bottomRight), ImGui::GetID(&mID)) || mIsSelected)
		{
			// If hovered or selected directory
			const ImRect expandedArea = UI::RectExpanded(ImRect(topLeft, bottomRight), 2.0f, 2.0f);
			ImGui::PushClipRect(expandedArea.Min, expandedArea.Max, false);

			// Draw shadow
			drawShadow(topLeft, bottomRight, true);
			auto* drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(topLeft, bottomRight, Colors::Theme::groupHeader, 6.0f);
			ImGui::PopClipRect();
		}

		// Thumbnail
		//==========
		ImGui::PushID(mName.c_str());
		ImGui::InvisibleButton("##thumbnailButton", ImVec2{ thumbnailSize, thumbnailSize });
		UI::DrawButtonImage(mIcon, IM_COL32(255, 255, 255, 225),
			IM_COL32(255, 255, 255, 255),
			IM_COL32(255, 255, 255, 255),
			UI::RectExpanded(UI::GetItemRect(), -6.0f, -6.0f));

		// Info Panel
		//-----------
		auto renamingWidget = [&]
			{
				ImGui::SetKeyboardFocusHere();
				if (ImGui::InputText("##rename", sRenameBuffer, MAX_INPUT_BUFFER_LENGTH, ImGuiInputTextFlags_EnterReturnsTrue))
				{
					Rename(sRenameBuffer);
					mIsRenaming = false;
					result.Modify(ContentBrowserAction::Renamed, true);
				}
			};

		UI::ShiftCursor(edgeOffset, edgeOffset);
		
		if (mType == ItemType::Directory)
		{
			ImGui::BeginVertical((std::string("InfoPanel") + mName).c_str(), ImVec2(thumbnailSize - edgeOffset * 2.0f, infoPanelHeight - edgeOffset));
			{
				// Centre align directory name
				ImGui::BeginHorizontal(mName.c_str(), ImVec2(thumbnailSize - 2.0f, 0.0f));
				ImGui::Spring();
				{
					ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + (thumbnailSize - edgeOffset * 3.0f));

					const float textWidth = std::min(ImGui::CalcTextSize(mName.c_str()).x, thumbnailSize);
					ImGui::SetNextItemWidth(textWidth);
					if (mIsRenaming)
					{
						ImGui::SetNextItemWidth(thumbnailSize - edgeOffset * 3.0f);
						renamingWidget();
					}
					else
					{
						ImGui::SetNextItemWidth(textWidth);
						ImGui::Text(mName.c_str());
					}

					ImGui::PopTextWrapPos();
				}

				ImGui::Spring();
				ImGui::EndHorizontal();
				ImGui::Spring();
			}

			ImGui::EndVertical();
		}
		else
		{
			ImGui::BeginVertical((std::string("InfoPanel") + mName).c_str(), ImVec2(thumbnailSize - edgeOffset * 3.0f, infoPanelHeight - edgeOffset));
			{
				ImGui::BeginHorizontal("label", ImVec2(0.0f, 0.0f));				
				ImGui::SuspendLayout();
				ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + (thumbnailSize - edgeOffset * 2.0f));

				if (mIsRenaming)
				{
					ImGui::SetNextItemWidth(thumbnailSize - edgeOffset * 3.0f);
					renamingWidget();
				}
				else
				{
					ImGui::Text(mName.c_str());
				}

				ImGui::PopTextWrapPos();
				ImGui::ResumeLayout();

				ImGui::Spring();
				ImGui::EndHorizontal();
			}
			ImGui::Spring();

			if (displayAssetType)
			{
				ImGui::BeginHorizontal("assetType", ImVec2(0.0f, 0.0f));
				ImGui::Spring();
				{
					AssetMetadata& metadata = AssetManager::GetMetadata(mID);
					const std::string& assetType = Utils::ToUpper(Utils::AssetTypeToString(metadata.Type));
					UI::ScopedColor textColor(ImGuiCol_Text, Colors::Theme::textDarker);
					ImGui::TextUnformatted(assetType.c_str());
				}
				ImGui::EndHorizontal();
				ImGui::Spring(-1.0f, edgeOffset);
			}
			ImGui::EndVertical();
		}

		UI::ShiftCursor(-edgeOffset, -edgeOffset);
		if (!mIsRenaming)
		{
			if (Input::IsKeyPressed(KeyCode::F2) && mIsSelected)
			{
				StartRenaming();
			}
		}

		ImGui::PopStyleVar(); // ItemSpacing

		// End of the Item Group
		//======================
		ImGui::EndGroup();

		// Draw outline
		//-------------
		if (mIsSelected || ImGui::IsItemHovered())
		{
			ImRect itemRect = UI::GetItemRect();
			const ImRect expandedArea = UI::RectExpanded(itemRect, 2.0f, 2.0f);
			auto* drawList = ImGui::GetWindowDrawList();
			ImGui::PushClipRect(expandedArea.Min, expandedArea.Max, false);
			if (mIsSelected)
			{
				const bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsItemHovered();
				auto colTransition = UI::ColorWithMultipliedValue(Colors::Theme::selection, 0.8f);
				drawList->AddRect(itemRect.Min, itemRect.Max,
					mouseDown ? colTransition : Colors::Theme::selection, 6.0f,
					mType == ItemType::Directory ? 0 : ImDrawFlags_RoundCornersBottom, 1.0f);
			}
			else // isHovered
			{
				if (mType != ItemType::Directory)
				{
					drawList->AddRect(itemRect.Min, itemRect.Max,
						Colors::Theme::muted, 6.0f,
						ImDrawFlags_RoundCornersBottom, 1.0f);
				}
			}
			ImGui::PopClipRect();
		}

		// Mouse Events handling
		//======================
		UpdateDrop(result);
		bool dragging = false;
		if (dragging = ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			mIsDragging = true;

			const auto& selectionStack = ContentBrowserPanel::Get().GetSelectionStack();
			if (!selectionStack.IsSelected(mID))
			{
				result.Modify(ContentBrowserAction::ClearSelections, true);
			}

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
			else
			{
				const auto& selectionStack = ContentBrowserPanel::Get().GetSelectionStack();
				bool action = selectionStack.SelectionCount() > 1 ? ImGui::IsMouseReleased(ImGuiMouseButton_Left) : ImGui::IsMouseClicked(ImGuiMouseButton_Left);
				bool skipBecauseDragging = mIsDragging && selectionStack.IsSelected(mID);
				if (action && !skipBecauseDragging)
				{
					result.Modify(ContentBrowserAction::Selected, true);

					if (!Input::IsKeyPressed(KeyCode::LeftControl) && !Input::IsKeyPressed(KeyCode::LeftShift))
						result.Modify(ContentBrowserAction::ClearSelections, true);

					if (Input::IsKeyPressed(KeyCode::LeftShift))
						result.Modify(ContentBrowserAction::SelectToHere, true);
				}
			}
		}

		if (ImGui::BeginPopupContextItem("CBItemContextMenu"))
		{
			result.Modify(ContentBrowserAction::Selected, true);

			if (!Input::IsKeyPressed(KeyCode::LeftControl) && !Input::IsKeyPressed(KeyCode::LeftShift))
			{
				result.Modify(ContentBrowserAction::ClearSelections, true);
			}

			if (Input::IsKeyPressed(KeyCode::LeftShift))
			{
				result.Modify(ContentBrowserAction::SelectToHere, true);
			}

			ContextMenuOpen(result);
			ImGui::EndPopup();
		}

		ImGui::PopID();

		mIsDragging = dragging;

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
	}

	void ContentBrowserItem::ContextMenuOpen(CBItemActionResult& actionResult)
	{
		if (ImGui::MenuItem("Reload"))
		{
			actionResult.Modify(ContentBrowserAction::Reload, true);
		}

		if (ImGui::MenuItem("Rename"))
		{
			StartRenaming();
		}
		if (ImGui::MenuItem("Delete"))
		{
			actionResult.Modify(ContentBrowserAction::OpenDeleteDialogue, true);
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Show In Explorer"))
		{
			actionResult.Modify(ContentBrowserAction::ShowInExplorer, true);
		}
		if (ImGui::MenuItem("Open Externally"))
		{
			actionResult.Modify(ContentBrowserAction::OpenExternal, true);
		}

		RenderCustomContextItems();
	}

	void ContentBrowserItem::RenderEnd()
	{
		ImGui::PopID();
		ImGui::NextColumn();
	}

	ContentBrowserDirectory::ContentBrowserDirectory(const Ref<DirectoryInfo>& directoryInfo, const Ref<Texture2D>& icon)
		: ContentBrowserItem(ContentBrowserItem::ItemType::Directory, directoryInfo->Handle, directoryInfo->FilePath.filename().string(), icon), mDirectoryInfo(directoryInfo)
	{
	}

	void ContentBrowserDirectory::Activate(CBItemActionResult& actionResult)
	{
		actionResult.Modify(ContentBrowserAction::NavigateToThis, true);
	}

	void ContentBrowserDirectory::Renamed(const std::string& newName, bool fromCallback)
	{
		if (!fromCallback)
		{
			if (FileSystem::Exists(Project::GetActive()->GetAssetDirectory() / mDirectoryInfo->FilePath.parent_path() / newName))
			{
				NR_CORE_ERROR("A directory with that name already exists!");
				return;
			}

			FileSystem::Rename(Project::GetActive()->GetAssetDirectory() / mDirectoryInfo->FilePath, Project::GetActive()->GetAssetDirectory() / mDirectoryInfo->FilePath.parent_path() / newName);
		}
		mName = newName;
		UpdateDirectoryPath(mDirectoryInfo, mDirectoryInfo->FilePath.parent_path(), newName);
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
		bool deleted = FileSystem::DeleteFile(Project::GetActive()->GetAssetDirectory() / mDirectoryInfo->FilePath);
		if (!deleted)
		{
			NR_CORE_ERROR("Failed to delete folder {0}", mDirectoryInfo->FilePath);
			return;
		}

		for (auto asset : mDirectoryInfo->Assets)
		{
			AssetManager::AssetDeleted(asset);
		}
	}

	bool ContentBrowserDirectory::Move(const std::filesystem::path& destination)
	{
		bool wasMoved = FileSystem::MoveFile(Project::GetActive()->GetAssetDirectory() / mDirectoryInfo->FilePath, Project::GetActive()->GetAssetDirectory() / destination);
		if (!wasMoved)
		{
			return false;
		}

		UpdateDirectoryPath(mDirectoryInfo, destination, mDirectoryInfo->FilePath.filename());

		auto& parentSubdirs = mDirectoryInfo->Parent->SubDirectories;
		parentSubdirs.erase(parentSubdirs.find(mDirectoryInfo->Handle));

		auto newParent = ContentBrowserPanel::Get().GetDirectory(destination);
		newParent->SubDirectories[mDirectoryInfo->Handle] = mDirectoryInfo;
		mDirectoryInfo->Parent = newParent;

		return true;
	}

	void ContentBrowserDirectory::UpdateDirectoryPath(Ref<DirectoryInfo> directoryInfo, const std::filesystem::path& newParentPath, const std::filesystem::path& newName)
	{
		directoryInfo->FilePath = newParentPath / newName;
		for (auto assetHandle : directoryInfo->Assets)
		{
			auto& metadata = AssetManager::GetMetadata(assetHandle);
			metadata.FilePath = directoryInfo->FilePath / metadata.FilePath.filename();
		}

		for (auto [handle, subdirectory] : directoryInfo->SubDirectories)
		{
			UpdateDirectoryPath(subdirectory, directoryInfo->FilePath, subdirectory->FilePath.filename());
		}
	}

	ContentBrowserAsset::ContentBrowserAsset(const AssetMetadata& assetInfo, const Ref<Texture2D>& icon)
		: ContentBrowserItem(ContentBrowserItem::ItemType::Asset, assetInfo.Handle, assetInfo.FilePath.stem().string(), icon), mAssetInfo(assetInfo)
	{
	}

	void ContentBrowserAsset::Delete()
	{
		auto filepath = AssetManager::GetFileSystemPath(mAssetInfo);
		bool deleted = FileSystem::DeleteFile(filepath);
		if (!deleted)
		{
			NR_CORE_ERROR("Couldn't delete {0}", mAssetInfo.FilePath.string());
			return;
		}

		auto currentDirectory = ContentBrowserPanel::Get().GetDirectory(mAssetInfo.FilePath.parent_path());
		currentDirectory->Assets.erase(std::remove(currentDirectory->Assets.begin(), currentDirectory->Assets.end(), mAssetInfo.Handle), currentDirectory->Assets.end());
		
		AssetManager::AssetDeleted(mAssetInfo.Handle);
	}

	bool ContentBrowserAsset::Move(const std::filesystem::path& destination)
	{
		auto filepath = AssetManager::GetFileSystemPath(mAssetInfo);
		bool wasMoved = FileSystem::MoveFile(filepath, Project::GetActive()->GetAssetDirectory() / destination);
		if (!wasMoved)
		{
			NR_CORE_ERROR("Couldn't move {0} to {1}", mAssetInfo.FilePath.string(), destination.string());
			return false;
		}

		auto currentDirectory = ContentBrowserPanel::Get().GetDirectory(mAssetInfo.FilePath.parent_path());
		currentDirectory->Assets.erase(std::remove(currentDirectory->Assets.begin(), currentDirectory->Assets.end(), mAssetInfo.Handle), currentDirectory->Assets.end());

		auto destinationDirectory = ContentBrowserPanel::Get().GetDirectory(destination);
		destinationDirectory->Assets.push_back(mAssetInfo.Handle);
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
		auto filepath = AssetManager::GetFileSystemPath(mAssetInfo);
		std::filesystem::path newFilepath = fmt::format("{0}\\{1}{2}", filepath.parent_path().string(), newName, filepath.extension().string());

		if (!fromCallback)
		{
			if (!FileSystem::Rename(filepath, newFilepath))
			{
				NR_CORE_ERROR("A file with that name already exists!");
				return;
			}

			AssetManager::AssetRenamed(mAssetInfo.Handle, AssetManager::GetRelativePath(newFilepath));
		}

		mName = newName;
		mAssetInfo.FilePath = AssetManager::GetRelativePath(newFilepath);
	}
}