#pragma once

#include <map>

#include "NotRed/Util/AssetManager.h"
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
		void RenderFileListView(int dirIndex);
		void RenderFileGridView(int dirIndex);
		void HandleDragDrop(RendererID icon, int dirIndex);
		void RenderDirectoriesListView(int dirIndex);
		void RenderDirectoriesGridView(int dirIndex);
		void RenderBreadCrumbs();
		void RenderSearch();
		void RenderBottom();

		ImGuiInputTextCallback SearchCallback(ImGuiInputTextCallbackData* data);

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

		std::string mCurrentDirPath;
		std::string mBaseDirPath;
		std::string mPrevDirPath;
		std::string mMovePath;

		std::string mForwardPath;
		std::string mBackPath;

		int mBasePathLen;
		int mDirDataLen;

		bool mIsDragging = false;
		bool mDisplayListView = false;
		bool mUpdateBreadCrumbs = true;
		bool mShowSearchBar = false;
		bool mIsPathChanged = false;

		char mInputBuffer[1024];

		std::vector<DirectoryInfo> mCurrentDir;
		std::vector<DirectoryInfo> mBaseProjectDir;

		ImGuiInputTextCallbackData mData;
		std::map<size_t, Ref<Texture2D>> mAssetIconMaps;
		//NotificationManager nManager;
		AssetManager mAssetManager;
	};

}