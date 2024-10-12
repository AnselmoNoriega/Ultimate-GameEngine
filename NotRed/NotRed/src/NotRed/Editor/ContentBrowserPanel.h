#pragma once

#include <map>

#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Renderer/Texture.h"
#include "NotRed/ImGui/ImGui.h"

#define MAX_INPUT_BUFFER_LENGTH 128

namespace NR
{
	struct SelectionStack
	{
	public:
		void Select(AssetHandle item)
		{
			mSelections.push_back(item);
		}

		void Deselect(AssetHandle item)
		{
			for (auto it = mSelections.begin(); it != mSelections.end(); it++)
			{
				if (*it == item)
				{
					mSelections.erase(it);
					break;
				}
			}
		}

		bool IsSelected(AssetHandle item) const
		{
			if (mSelections.size() == 0)
			{
				return false;
			}

			for (auto selection : mSelections)
			{
				if (selection == item)
				{
					return true;
				}
			}

			return false;
		}

		void Clear()
		{
			if (mSelections.size() > 0)
			{
				mSelections.clear();
			}
		}

		size_t SelectionCount() const
		{
			return mSelections.size();
		}

		AssetHandle* GetSelectionData()
		{
			return mSelections.data();
		}

		AssetHandle operator[](size_t index) const
		{
			NR_CORE_ASSERT(index >= 0 && index < mSelections.size());
			return mSelections[index];
		}

	private:
		std::vector<AssetHandle> mSelections;
	};

	struct DirectoryInfo : public RefCounted
	{
		AssetHandle Handle;
		AssetHandle Parent;
		std::string Name;
		std::string FilePath;
		std::vector<AssetHandle> Assets;
		std::vector<AssetHandle> SubDirectories;
	};

	struct SearchResults
	{
		std::vector<Ref<DirectoryInfo>> Directories;
		std::vector<AssetMetadata> Assets;
		void Append(const SearchResults& other)
		{
			Directories.insert(Directories.end(), other.Directories.begin(), other.Directories.end());
			Assets.insert(Assets.end(), other.Assets.begin(), other.Assets.end());
		}
	};

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();

		void ImGuiRender();

	private:
		AssetHandle ProcessDirectory(const std::string& directoryPath, AssetHandle parent);

		void DrawDirectoryInfo(AssetHandle directory);

		void RenderDirectory(Ref<DirectoryInfo>& directory);
		void RenderAsset(AssetMetadata& assetInfo);
		void HandleDragDrop(Ref<Image2D> icon, AssetHandle assetHandle);

		void RenderBreadCrumbs();
		void RenderBottomBar();

		void HandleDirectoryRename(Ref<DirectoryInfo>& dirInfo);
		void HandleAssetRename(AssetMetadata& asset);

		void UpdateCurrentDirectory(AssetHandle directoryHandle);

		void FileSystemChanged(FileSystemChangedEvent e);
		void AssetDeleted(const FileSystemChangedEvent& e);
		void RemoveDirectory(Ref<DirectoryInfo> dirInfo);
		
		void DirectoryAdded(const std::string& directoryPath);
		Ref<DirectoryInfo> GetDirectoryInfo(const std::string& filepath) const;

		SearchResults Search(const std::string& query, AssetHandle directoryHandle);

	private:
		Ref<Texture2D> mFileTex;
		Ref<Texture2D> mFolderIcon;
		Ref<Texture2D> mBackbtnTex;
		Ref<Texture2D> mFwrdbtnTex;

		bool mIsDragging = false;
		bool mUpdateBreadCrumbs = true;
		bool mIsAnyItemHovered = false;
		bool mUpdateDirectoryNextFrame = false;		
		bool mRenamingSelected = false;

		char mRenameBuffer[MAX_INPUT_BUFFER_LENGTH];
		char mSearchBuffer[MAX_INPUT_BUFFER_LENGTH];

		AssetHandle mCurrentDirHandle;
		AssetHandle mBaseDirectoryHandle;
		AssetHandle mPrevDirHandle;
		AssetHandle mNextDirHandle;

		Ref<DirectoryInfo> mCurrentDirectory;
		Ref<DirectoryInfo> mBaseDirectory;

		std::vector<Ref<DirectoryInfo>> mCurrentDirectories;
		std::vector<Ref<DirectoryInfo>> mBreadCrumbData;
		std::vector<AssetMetadata> mCurrentAssets;

		SelectionStack mSelectedAssets;

		std::map<std::string, Ref<Texture2D>> mAssetIconMap;

		std::unordered_map<AssetHandle, Ref<DirectoryInfo>> mDirectories;
	};

}