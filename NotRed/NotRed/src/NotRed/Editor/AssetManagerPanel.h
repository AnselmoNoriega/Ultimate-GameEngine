#pragma once

#include <map>

#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Renderer/Texture.h"
#include "NotRed/ImGui/ImGui.h"

#define MAX_INPUT_BUFFER_LENGTH 128

namespace NR
{
	template<typename T>
	struct SelectionStack
	{
	public:
		void Select(T item)
		{
			mSelections.push_back(item);
		}

		void Deselect(T item)
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

		bool IsSelected(T item) const
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

		T* GetSelectionData()
		{
			return mSelections.data();
		}

	private:
		std::vector<T> mSelections;
	};

	class AssetManagerPanel
	{
	public:
		AssetManagerPanel();

		void ImGuiRender();

	private:
		void DrawDirectoryInfo(AssetHandle directory);

		void RenderAsset(Ref<Asset>& assetHandle);
		void HandleDragDrop(RendererID icon, Ref<Asset>& asset);
		void RenderBreadCrumbs();

		void HandleRenaming(Ref<Asset>& asset);

		void UpdateCurrentDirectory(AssetHandle directoryHandle);

	private:
		Ref<Texture2D> mFileTex;
		Ref<Texture2D> mBackbtnTex;
		Ref<Texture2D> mFwrdbtnTex;
		Ref<Texture2D> mFolderRightTex;
		Ref<Texture2D> mSearchTex;

		std::string mMovePath;

		bool mIsDragging = false;
		bool mUpdateBreadCrumbs = true;
		bool mIsAnyItemHovered = false;
		bool mUpdateDirectoryNextFrame = false;

		char mInputBuffer[MAX_INPUT_BUFFER_LENGTH];

		AssetHandle mCurrentDirHandle;
		AssetHandle mBaseDirectoryHandle;
		AssetHandle mPrevDirHandle;
		AssetHandle mNextDirHandle;
		Ref<Directory> mCurrentDirectory;
		Ref<Directory> mBaseDirectory;
		std::vector<Ref<Asset>> mCurrentDirAssets;

		std::vector<Ref<Directory>> mBreadCrumbData;

		AssetHandle mDraggedAssetID = 0;
		SelectionStack<AssetHandle> mSelectedAssets;
		bool mRenamingSelected = false;

		std::map<std::string, Ref<Texture2D>> mAssetIconMap;
	};

}