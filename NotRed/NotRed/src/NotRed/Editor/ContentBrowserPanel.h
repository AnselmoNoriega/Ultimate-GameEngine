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
			for (auto selection : mSelections)
			{
				if (selection == item)
					return true;
			}

			return false;
		}

		void Clear()
		{
			mSelections.clear();
		}

		size_t SelectionCount() const
		{
			return mSelections.size();
		}

		AssetHandle* GetSelectionData()
		{
			return mSelections.data();
		}

	private:
		std::vector<AssetHandle> mSelections;
	};

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();

		void ImGuiRender();

	private:
		void DrawDirectoryInfo(AssetHandle directory);

		void RenderAsset(Ref<Asset>& assetHandle);
		void HandleDragDrop(Ref<Image2D> icon, Ref<Asset>& asset);
		void RenderBreadCrumbs();

		void HandleRenaming(Ref<Asset>& asset);

		void UpdateCurrentDirectory(AssetHandle directoryHandle);

	private:
		Ref<Texture2D> mFileTex;
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
		Ref<Directory> mCurrentDirectory;
		Ref<Directory> mBaseDirectory;

		std::vector<Ref<Asset>> mCurrentDirFiles;
		std::vector<Ref<Asset>> mCurrentDirFolders;
		std::vector<Ref<Directory>> mBreadCrumbData;

		SelectionStack mSelectedAssets;

		std::map<std::string, Ref<Texture2D>> mAssetIconMap;
	};

}