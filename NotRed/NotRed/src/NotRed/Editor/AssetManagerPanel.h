#pragma once

#include <map>

#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Renderer/Texture.h"
#include "NotRed/ImGui/ImGui.h"

namespace NR
{
	class AssetManagerPanel
	{
	public:
		AssetManagerPanel();
		void ImGuiRender();

	private:
		void DrawDirectoryInfo(DirectoryInfo& dir);

		void RenderFileListView(Ref<Asset>& asset);
		void RenderFileGridView(Ref<Asset>& asset);
		void HandleDragDrop(RendererID icon, Ref<Asset>& asset);
		void RenderDirectoriesListView(DirectoryInfo& dirInfo);
		void RenderDirectoriesGridView(DirectoryInfo& dirInfo);
		void RenderBreadCrumbs();
		void RenderBottom();

		void UpdateCurrentDirectory(int dirIndex);

		//ImGuiInputTextCallback SearchCallback(ImGuiInputTextCallbackData* data);

	private:
		Ref<Texture2D> mFolderTex;
		Ref<Texture2D> mFavoritesTex;
		Ref<Texture2D> mFileTex;
		Ref<Texture2D> mGoBackTex;
		Ref<Texture2D> mScriptTex;
		Ref<Texture2D> mResourceTex;
		Ref<Texture2D> mSceneTex;

		Ref<Texture2D> mBackbtnTex;
		Ref<Texture2D> mFwrdbtnTex;
		Ref<Texture2D> mFolderRightTex;
		Ref<Texture2D> mTagsTex;
		Ref<Texture2D> mSearchTex;
		Ref<Texture2D> mGridView;
		Ref<Texture2D> mListView;

		std::string mMovePath;

		int mBaseDirIndex;
		int mCurrentDirIndex;
		int mPrevDirIndex;
		int mNextDirIndex;

		bool mIsDragging = false;
		bool mDisplayListView = false;
		bool mUpdateBreadCrumbs = true;
		bool mShowSearchBar = false;
		bool mDirectoryChanged = false;

		char mInputBuffer[1024];

		DirectoryInfo mCurrentDir;
		DirectoryInfo mBaseProjectDir;
		std::vector<DirectoryInfo> mCurrentDirChildren;
		std::vector<Ref<Asset>> mCurrentDirAssets;

		std::vector<DirectoryInfo> mBreadCrumbData;

		AssetHandle mDraggedAssetID = 0;

		ImGuiInputTextCallbackData mData;
		std::map<size_t, Ref<Texture2D>> mAssetIconMap;
	};

}