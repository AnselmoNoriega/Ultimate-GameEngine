#include "nrpch.h"
#include "AssetManagerPanel.h"

#include "NotRed/Core/Application.h"
#include "NotRed/Util/DragDropData.h"

namespace NR
{
	AssetManagerPanel::AssetManagerPanel()
		: mAssetManager([&]() { mCurrentDir = mAssetManager.GetDirectoryContents(mCurrentDirPath); })
	{
		AssetTypes::Init();

		mFolderTex = Texture2D::Create("Assets/Editor/folder.png");
		mFavoritesTex = Texture2D::Create("Assets/Editor/favourites.png");
		mGoBackTex = Texture2D::Create("Assets/Editor/back.png");
		mScriptTex = Texture2D::Create("Assets/Editor/script.png");
		mResourceTex = Texture2D::Create("Assets/Editor/resource.png");
		mSceneTex = Texture2D::Create("Assets/Editor/scene.png");

		mAssetIconMaps[-1] = Texture2D::Create("Assets/Editor/file.png");
		mAssetIconMaps[AssetTypes::GetAssetTypeID("fbx")] = Texture2D::Create("Assets/Editor/fbx.png");
		mAssetIconMaps[AssetTypes::GetAssetTypeID("obj")] = Texture2D::Create("Assets/Editor/obj.png");
		mAssetIconMaps[AssetTypes::GetAssetTypeID("wav")] = Texture2D::Create("Assets/Editor/wav.png");
		mAssetIconMaps[AssetTypes::GetAssetTypeID("cs")] = Texture2D::Create("Assets/Editor/csc.png");
		mAssetIconMaps[AssetTypes::GetAssetTypeID("png")] = Texture2D::Create("Assets/Editor/png.png");
		mAssetIconMaps[AssetTypes::GetAssetTypeID("blend")] = Texture2D::Create("Assets/Editor/blend.png");
		mAssetIconMaps[AssetTypes::GetAssetTypeID("hsc")] = Texture2D::Create("Assets/Editor/hazel.png");

		mBackbtnTex = Texture2D::Create("Assets/Editor/btn_back.png");
		mFwrdbtnTex = Texture2D::Create("Assets/Editor/btn_fwrd.png");
		mFolderRightTex = Texture2D::Create("Assets/Editor/folder_hierarchy.png");
		mSearchTex = Texture2D::Create("Assets/Editor/search.png");
		mTagsTex = Texture2D::Create("Assets/Editor/tags.png");
		mGridView = Texture2D::Create("Assets/Editor/grid.png");
		mListView = Texture2D::Create("Assets/Editor/list.png");

		mBaseDirPath = "Assets";
		mCurrentDirPath = mBaseDirPath;
		mPrevDirPath = mCurrentDirPath;
		mBaseProjectDir = mAssetManager.GetDirectoryContents(mBaseDirPath);
		mCurrentDir = mBaseProjectDir;
		mBasePathLen = strlen(mBaseDirPath.c_str());
		mDirDataLen = 0;

		memset(mInputBuffer, 0, sizeof(mInputBuffer));
	}

	void AssetManagerPanel::ImGuiRender()
	{
		ImGui::Begin("Project", NULL, ImGuiWindowFlags_None);
		{
			UI::BeginPropertyGrid();
			ImGui::SetColumnOffset(1, 240);

			ImGui::BeginChild("##folders_common");
			{
				if (ImGui::CollapsingHeader("res://", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
				{
					if (ImGui::TreeNode("Contents"))
					{
						for (int i = 0; i < mBaseProjectDir.size(); ++i)
						{
							if (ImGui::TreeNode(mBaseProjectDir[i].Filename.c_str()))
							{
								auto dirData = mAssetManager.GetDirectoryContents(mBaseProjectDir[i].AbsolutePath);
								for (int i = 0; i < dirData.size(); ++i)
								{
									if (!dirData[i].IsFile)
									{
										if (ImGui::TreeNode(dirData[i].Filename.c_str()))
											ImGui::TreePop();
									}
									else
									{
										std::string parentDir = mAssetManager.GetParentPath(dirData[i].AbsolutePath);
										ImGui::Indent();
										ImGui::Selectable(dirData[i].Filename.c_str(), false);
										ImGui::Unindent();
									}
								}
								ImGui::TreePop();
							}

							if (mIsDragging && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
							{
								mMovePath = mBaseProjectDir[i].AbsolutePath.c_str();
							}
						}
						ImGui::TreePop();
					}

					if (ImGui::IsMouseDown(1))
					{

					}
				}

				ImGui::EndChild();
			}

			if (ImGui::BeginDragDropTarget())
			{
				auto data = ImGui::AcceptDragDropPayload("selectable", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
				if (data)
				{
					std::string file = (char*)data->Data;
					if (mAssetManager.MoveFile(file, mMovePath))
					{
						NR_CORE_INFO("Moved File: " + file + " to " + mMovePath);
						mCurrentDir = mAssetManager.GetDirectoryContents(mCurrentDirPath);
					}
					mIsDragging = false;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::NextColumn();

			ImGui::BeginChild("##directory_structure", ImVec2(ImGui::GetColumnWidth() - 12, ImGui::GetWindowHeight() - 50));
			{
				RenderBreadCrumbs();
				ImGui::SameLine();
				ImGui::Dummy(ImVec2(ImGui::GetColumnWidth() - 350, 0));
				ImGui::SameLine();
				RenderSearch();
				ImGui::EndChild();

				ImGui::BeginChild("Scrolling");

				if (!mDisplayListView)
				{
					ImGui::Columns(10, nullptr, false);
				}

				for (int i = 0; i < mCurrentDir.size(); ++i)
				{
					if (mCurrentDir.size() > 0)
					{
						if (!mDisplayListView)
						{
							mCurrentDir[i].IsFile ? RenderFileGridView(i) : RenderDirectoriesGridView(i);
						}
						else
						{
							mCurrentDir[i].IsFile ? RenderFileListView(i) : RenderDirectoriesListView(i);
						}

						ImGui::NextColumn();
					}
				}

				if (ImGui::BeginPopupContextWindow(0, 1))
				{
					if (ImGui::BeginMenu("New"))
					{
						if (ImGui::MenuItem("Folder"))
						{
						}

						if (ImGui::MenuItem("Scene"))
						{
						}

						if (ImGui::MenuItem("Script"))
						{
						}

						if (ImGui::MenuItem("Prefab"))
						{
						}

						if (ImGui::BeginMenu("Shaders"))
						{
							if (ImGui::MenuItem("Shader"))
							{
							}

							if (ImGui::MenuItem("Shader Graph"))
							{
							}

							ImGui::EndMenu();
						}

						ImGui::EndMenu();
					}

					ImGui::EndPopup();
				}

				ImGui::EndChild();
				ImGui::EndChild();
			}

			if (ImGui::BeginDragDropTarget())
			{
				auto data = ImGui::AcceptDragDropPayload("selectable", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
				if (data)
				{
					std::string a = (char*)data->Data;
					if (mAssetManager.MoveFile(a, mMovePath))
					{
						NR_CORE_INFO("Moved File: " + a + " to " + mMovePath);
					}
					mIsDragging = false;
				}
				ImGui::EndDragDropTarget();
			}

			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("Create"))
				{
					if (ImGui::MenuItem("Import New Asset", "Ctrl + O"))
					{
						std::string filename = Application::Get().OpenFile("");
						mAssetManager.ProcessAsset(filename);
					}

					if (ImGui::MenuItem("Refresh", "Ctrl + R"))
					{
						mBaseProjectDir = mAssetManager.GetDirectoryContents(mBaseDirPath);
						mCurrentDir = mAssetManager.GetDirectoryContents(mCurrentDirPath);
					}

					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

			UI::EndPropertyGrid();
			ImGui::End();
		}
	}

	void AssetManagerPanel::RenderFileListView(int dirIndex)
	{
		size_t fileID = AssetTypes::GetAssetTypeID(mCurrentDir[dirIndex].FileType);
		RendererID iconRef = mAssetIconMaps[fileID]->GetRendererID();
		ImGui::Image((ImTextureID)iconRef, ImVec2(20, 20));

		ImGui::SameLine();

		if (ImGui::Selectable(mCurrentDir[dirIndex].Filename.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
		{
			if (ImGui::IsMouseDoubleClicked(0))
			{
				mAssetManager.HandleAsset(mCurrentDir[dirIndex].AbsolutePath);
			}
		}

		HandleDragDrop(iconRef, dirIndex);
	}

	void AssetManagerPanel::RenderFileGridView(int dirIndex)
	{
		ImGui::BeginGroup();

		size_t fileID = AssetTypes::GetAssetTypeID(mCurrentDir[dirIndex].FileType);
		RendererID iconRef = mAssetIconMaps[fileID]->GetRendererID();
		float columnWidth = ImGui::GetColumnWidth();

		ImGui::ImageButton((ImTextureID)iconRef, { columnWidth - 10.0f, columnWidth - 10.0f });

		HandleDragDrop(iconRef, dirIndex);

		std::string newFileName = mAssetManager.StripExtras(mCurrentDir[dirIndex].Filename);
		ImGui::TextWrapped(newFileName.c_str());
		ImGui::EndGroup();
	}

	void AssetManagerPanel::HandleDragDrop(RendererID icon, int dirIndex)
	{
		// Drag 'n' Drop Implementation For File Moving
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			ImGui::Image((ImTextureID)icon, ImVec2(20, 20));
			ImGui::SameLine();
			ImGui::Text(mCurrentDir[dirIndex].Filename.c_str());
			int size = sizeof(const char*) + strlen(mCurrentDir[dirIndex].AbsolutePath.c_str());
			ImGui::SetDragDropPayload("selectable", mCurrentDir[dirIndex].AbsolutePath.c_str(), size);
			mIsDragging = true;
			ImGui::EndDragDropSource();
		}

		// Drag 'n' Drop Implementation For Asset Handling In Scene Viewport
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			ImGui::Image((ImTextureID)icon, ImVec2(20, 20));
			ImGui::SameLine();
			ImGui::Text(mCurrentDir[dirIndex].Filename.c_str());

			AssetType assetType = AssetTypes::GetAssetTypeFromExtension(mCurrentDir[dirIndex].FileType);

			if (assetType == AssetType::Mesh)
			{
				const char* sourceType = mCurrentDir[dirIndex].AbsolutePath.c_str();
				const char* name = mCurrentDir[dirIndex].Filename.c_str();
				const char* type = "Mesh";

				DragDropData d(type, sourceType, name);
				ImGui::SetDragDropPayload("scene_entity_AssetsP", &d, sizeof(d));
				mIsDragging = true;
			}

			if (assetType == AssetType::Scene)
			{
				const char* sourceType = mCurrentDir[dirIndex].AbsolutePath.c_str();
				const char* name = mCurrentDir[dirIndex].Filename.c_str();
				const char* type = "HazelScene";

				DragDropData d(type, sourceType, name);
				ImGui::SetDragDropPayload("scene_entity_AssetsP", &d, sizeof(d));
				mIsDragging = true;
			}

			ImGui::EndDragDropSource();
		}
	}

	void AssetManagerPanel::RenderDirectoriesListView(int dirIndex)
	{
		ImGui::Image((ImTextureID)mFolderTex->GetRendererID(), ImVec2(20, 20));
		ImGui::SameLine();

		if (ImGui::Selectable(mCurrentDir[dirIndex].Filename.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
		{
			if (ImGui::IsMouseDoubleClicked(0))
			{
				mPrevDirPath = mCurrentDir[dirIndex].AbsolutePath;
				mCurrentDirPath = mCurrentDir[dirIndex].AbsolutePath;
				mCurrentDir = mAssetManager.GetDirectoryContents(mCurrentDir[dirIndex].AbsolutePath);
			}
		}

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
		{
			ImGui::Image((ImTextureID)mFolderTex->GetRendererID(), ImVec2(20, 20));
			ImGui::SameLine();
			ImGui::Text(mCurrentDir[dirIndex].Filename.c_str());
			int size = sizeof(const char*) + strlen(mCurrentDir[dirIndex].AbsolutePath.c_str());
			ImGui::SetDragDropPayload("selectable", mCurrentDir[dirIndex].AbsolutePath.c_str(), size);
			mIsDragging = true;
			ImGui::EndDragDropSource();
		}
	}

	void AssetManagerPanel::RenderDirectoriesGridView(int dirIndex)
	{
		ImGui::BeginGroup();

		float columnWidth = ImGui::GetColumnWidth();
		ImGui::ImageButton((ImTextureID)mFolderTex->GetRendererID(), { columnWidth - 10.0f, columnWidth - 10.0f });

		if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
		{
			mPrevDirPath = mCurrentDir[dirIndex].AbsolutePath;
			mCurrentDirPath = mCurrentDir[dirIndex].AbsolutePath;
			mCurrentDir = mAssetManager.GetDirectoryContents(mCurrentDir[dirIndex].AbsolutePath);
			mIsPathChanged = true;
			mDirDataLen = mCurrentDir.size();
		}

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
		{
			ImGui::Image((ImTextureID)mFolderTex->GetRendererID(), ImVec2(20, 20));
			ImGui::SameLine();
			ImGui::Text(mCurrentDir[dirIndex].Filename.c_str());
			int size = sizeof(const char*) + strlen(mCurrentDir[dirIndex].AbsolutePath.c_str());
			ImGui::SetDragDropPayload("selectable", mCurrentDir[dirIndex].AbsolutePath.c_str(), size);
			mIsDragging = true;
			ImGui::EndDragDropSource();
		}

		if (!mIsPathChanged)
		{
			auto fname = mCurrentDir[dirIndex].Filename;
			auto newFname = mAssetManager.StripExtras(fname);
			ImGui::TextWrapped(newFname.c_str());
		}
		else
		{
			for (int i = 0; i < mDirDataLen; ++i)
			{
				auto fname = mCurrentDir[i].Filename;
				auto newFname = mAssetManager.StripExtras(fname);
				ImGui::TextWrapped(newFname.c_str());
				mIsPathChanged = false;
				mDirDataLen = 0;
			}
		}

		ImGui::EndGroup();

	}

	void AssetManagerPanel::RenderBreadCrumbs()
	{
		ImGui::BeginChild("##directory_breadcrumbs", ImVec2(ImGui::GetColumnWidth() - 100, 30));
		{
			RendererID viewTexture = mDisplayListView ? mListView->GetRendererID() : mGridView->GetRendererID();
			if (ImGui::ImageButton((ImTextureID)viewTexture, ImVec2(20, 18)))
			{
				mDisplayListView = !mDisplayListView;
			}

			ImGui::SameLine();

			if (ImGui::ImageButton((ImTextureID)mSearchTex->GetRendererID(), ImVec2(20, 18)))
				mShowSearchBar = !mShowSearchBar;

			ImGui::SameLine();

			if (mShowSearchBar)
			{
				ImGui::SameLine();
				ImGui::PushItemWidth(200);

				if (ImGui::InputTextWithHint("", "Search...", mInputBuffer, 100))
				{
					if (strlen(mInputBuffer) == 0)
					{
						mCurrentDir = mAssetManager.GetDirectoryContents(mCurrentDirPath);
					}
					else
					{
						mCurrentDir = mAssetManager.SearchFiles(mInputBuffer, mCurrentDirPath);
					}
				}

				ImGui::PopItemWidth();
				ImGui::SameLine();
			}

			if (ImGui::ImageButton((ImTextureID)mBackbtnTex->GetRendererID(), ImVec2(20, 18)))
			{
				if (strlen(mCurrentDirPath.c_str()) == mBasePathLen) return;
				mForwardPath = mCurrentDirPath;
				mBackPath = mAssetManager.GetParentPath(mCurrentDirPath);
				mCurrentDir = mAssetManager.GetDirectoryContents(mBackPath);
				mCurrentDirPath = mBackPath;
			}

			ImGui::SameLine();

			if (ImGui::ImageButton((ImTextureID)mFwrdbtnTex->GetRendererID(), ImVec2(20, 18)))
			{
				mCurrentDir = mAssetManager.GetDirectoryContents(mForwardPath);
				mCurrentDirPath = mForwardPath;
			}

			ImGui::SameLine();

			auto data = mAssetManager.GetDirectoryNames(mCurrentDirPath);

			for (int i = 0; i < data.size(); ++i)
			{
				if (data[i] != mBaseDirPath)
				{
					ImGui::Image((ImTextureID)mFolderRightTex->GetRendererID(), ImVec2(22, 23));
				}

				ImGui::SameLine();

				int size = strlen(data[i].c_str()) * 7;

				if (ImGui::Selectable(data[i].c_str(), false, 0, ImVec2(size, 22)))
				{
					int index = i + 1;
					std::string path = "";

					for (int e = 0; e < index; ++e)
					{
						path += data[e] + "/\\";
					}

					mCurrentDir = mAssetManager.GetDirectoryContents(path);
					mCurrentDirPath = path;
				}
				ImGui::SameLine();
			}

			ImGui::SameLine();

			ImGui::Dummy(ImVec2(ImGui::GetColumnWidth() - 400, 0));

			ImGui::SameLine();
		}
	}

	void AssetManagerPanel::RenderSearch()
	{
	}

	void AssetManagerPanel::RenderBottom()
	{
		ImGui::BeginChild("##nav", ImVec2(ImGui::GetColumnWidth() - 12, 23));
		{
			ImGui::EndChild();
		}
	}
}