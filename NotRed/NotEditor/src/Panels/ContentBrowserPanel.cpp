#include "nrpch.h"
#include "ContentBrowserPanel.h"

#include <filesystem>
#include <stack>

#include <imgui_internal.h>

#include "NotRed/Project/Project.h"
#include "NotRed/Core/Application.h"
#include "NotRed/Core/Input.h"

#include "NotRed/ImGui/CustomTreeNode.h"

#include "NotRed/Scene/Entity.h"
#include "NotRed/Scene/Prefab.h"

#include "NotRed/Editor/AssetEditorPanel.h"
#include "NotRed/Renderer/MaterialAsset.h"
#include "NotRed/Util/StringUtils.h"

#include "NotRed/Audio/Sound.h"
#include "NotRed/Editor/NodeGraphEditor/NodeGraphAsset.h"

namespace NR
{
	static int sColumnCount = 9;

	ContentBrowserPanel::ContentBrowserPanel()
		: mProject(nullptr)
	{
		sInstance = this;

		AssetManager::SetAssetChangeCallback(NR_BIND_EVENT_FN(ContentBrowserPanel::FileSystemChanged));

		TextureProperties clampProps;
		clampProps.SamplerWrap = TextureWrap::Clamp;
		mFolderIcon = Texture2D::Create("Resources/Editor/folder_closed.png", clampProps);
		mShadow = Texture2D::Create("Resources/Editor/NodeGraphEditor/shadow line_top.png", clampProps);
		mFileTex = Texture2D::Create("Resources/Editor/file.png");

		mAssetIconMap[".fbx"] = Texture2D::Create("Resources/Editor/fbx.png");
		mAssetIconMap[".obj"] = Texture2D::Create("Resources/Editor/obj.png");
		mAssetIconMap[".wav"] = Texture2D::Create("Resources/Editor/wav.png");
		mAssetIconMap[".cs"] = Texture2D::Create("Resources/Editor/csc.png");
		mAssetIconMap[".png"] = Texture2D::Create("Resources/Editor/png.png");
		mAssetIconMap[".nrmaterial"] = Texture2D::Create("Resources/Editor/MaterialAssetIcon.png");
		mAssetIconMap[".nrsc"] = Texture2D::Create("Resources/Editor/notred.png");
		mAssetIconMap[".nrprefab"] = Texture2D::Create("Resources/Editor/Icons/ContentBrowser/PrefabIcon.png");

		mAssetIconMap[".ttf"] = Texture2D::Create("Resources/Editor/Icons/ContentBrowser/FontIcon.png");
		mAssetIconMap[".ttc"] = mAssetIconMap.at(".ttf");
		mAssetIconMap[".otf"] = mAssetIconMap.at(".ttf");

		mBackbtnTex = Texture2D::Create("Resources/Editor/icon_back.png");
		mFwrdbtnTex = Texture2D::Create("Resources/Editor/icon_fwrd.png");
		mRefreshIcon = Texture2D::Create("Resources/Editor/refresh.png");

		memset(mSearchBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
	}

	AssetHandle ContentBrowserPanel::ProcessDirectory(const std::filesystem::path& directoryPath, const Ref<DirectoryInfo>& parent)
	{
		const auto& directory = GetDirectory(directoryPath);
		if (directory)
		{
			return directory->Handle;
		}

		Ref<DirectoryInfo> directoryInfo = Ref<DirectoryInfo>::Create();
		directoryInfo->Handle = AssetHandle();
		directoryInfo->Parent = parent;
		if (directoryPath == mProject->GetAssetDirectory())
		{
			directoryInfo->FilePath = "";
		}
		else
		{
			directoryInfo->FilePath = std::filesystem::relative(directoryPath, mProject->GetAssetDirectory());
		}

		for (auto entry : std::filesystem::directory_iterator(directoryPath))
		{
			if (entry.is_directory())
			{
				AssetHandle subdirHandle = ProcessDirectory(entry.path(), directoryInfo);
				directoryInfo->SubDirectories[subdirHandle] = mDirectories[subdirHandle];
				continue;
			}

			auto metadata = AssetManager::GetMetadata(std::filesystem::relative(entry.path(), mProject->GetAssetDirectory()));
			if (!metadata.IsValid())
			{
				AssetType type = AssetManager::GetAssetTypeFromPath(entry.path());
				if (type == AssetType::None)
				{
					continue;
				}
				metadata.Handle = AssetManager::ImportAsset(entry.path());
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
		if (!directory)
		{
			return;
		}

		mUpdateNavigationPath = true;
		mCurrentItems.Items.clear();

		for (auto& [subdirHandle, subdir] : directory->SubDirectories)
		{
			mCurrentItems.Items.push_back(Ref<ContentBrowserDirectory>::Create(subdir, mFolderIcon));
		}

		std::vector<AssetHandle> invalidAssets;
		for (auto assetHandle : directory->Assets)
		{
			AssetMetadata metadata = AssetManager::GetMetadata(assetHandle);
			if (metadata.IsValid())
			{
				mCurrentItems.Items.push_back(Ref<ContentBrowserAsset>::Create(metadata, mAssetIconMap.find(metadata.FilePath.extension().string()) != mAssetIconMap.end() ? mAssetIconMap[metadata.FilePath.extension().string()] : mFileTex));
			}
		}

		SortItemList();

		mPreviousDirectory = directory;
		mCurrentDirectory = directory;

		ClearSelections();
	}

	static float sPadding = 2.0f;
	static float sThumbnailSize = 128.0f;

	void ContentBrowserPanel::ImGuiRender(bool& isOpen)
	{
		ImGui::Begin("Content Browser", &isOpen, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);

		mIsContentBrowserHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
		mIsContentBrowserFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

		{
			//UI::BeginPropertyGrid();
			UI::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
			UI::ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
			UI::ScopedStyle cellPadding(ImGuiStyleVar_CellPadding, ImVec2(10.0f, 2.0f));
			ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable
				| ImGuiTableFlags_SizingFixedFit
				| ImGuiTableFlags_BordersInnerV;

			UI::PushID();

			if (ImGui::BeginTable(UI::GenerateID(), 2, tableFlags, ImVec2(0.0f, 0.0f)))
			{
				ImGui::TableSetupColumn("Outliner", 0, 300.0f);
				ImGui::TableSetupColumn("Directory Structure", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
			
				// Content Outliner
				ImGui::BeginChild("##folders_common");
				{
					if (ImGui::CollapsingHeader("Content", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
						UI::ScopedColorStack itemBg(ImGuiCol_Header, IM_COL32_DISABLE,
							ImGuiCol_HeaderActive, IM_COL32_DISABLE);

						std::vector<Ref<DirectoryInfo>> directories;
						directories.reserve(mBaseDirectory->SubDirectories.size());
						if (mBaseDirectory)
						{
							for (auto& [handle, directory] : mBaseDirectory->SubDirectories)
							{
								RenderDirectoryHierarchy(directory);
							}
						}

						std::sort(directories.begin(), directories.end(), [](const auto& a, const auto& b)
							{
								return a->FilePath.stem().string() < b->FilePath.stem().string();
							});

						for (auto& directory : directories)
						{
							RenderDirectoryHierarchy(directory);
						}
					}
					// Draw side shadow
					ImRect windowRect = UI::RectExpanded(ImGui::GetCurrentWindow()->Rect(), 0.0f, 10.0f);
					ImGui::PushClipRect(windowRect.Min, windowRect.Max, false);
					UI::DrawShadowInner(mShadow, 20.0f, windowRect, 1.0f, windowRect.GetHeight() / 4.0f, false, true, false, false);
					ImGui::PopClipRect();
				}
				ImGui::EndChild();

				ImGui::TableSetColumnIndex(1);

				// Directory Content
				const float topBarHeight = 26.0f;
				const float bottomBarHeight = 52.0f;
				ImGui::BeginChild("##directory_structure", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetWindowHeight() - topBarHeight - bottomBarHeight));
				{
					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
					RenderTopBar(topBarHeight);
					ImGui::PopStyleVar();
					ImGui::Separator();

					ImGui::BeginChild("Scrolling");
					{
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.35f));
						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));

						if (ImGui::BeginPopupContextWindow(0, 1))
						{
							if (ImGui::BeginMenu("New"))
							{
								if (ImGui::MenuItem("Folder"))
								{
									bool created = FileSystem::CreateDirectory(Project::GetAssetDirectory() / mCurrentDirectory->FilePath / "New Folder");
									if (created)
									{
										Refresh();

										const auto& directoryInfo = GetDirectory(mCurrentDirectory->FilePath / "New Folder");
										size_t index = mCurrentItems.FindItem(directoryInfo->Handle);
										if (index != ContentBrowserItemList::InvalidItem)
										{
											mCurrentItems[index]->StartRenaming();
										}
									}
								}

								if (ImGui::MenuItem("Scene"))
								{
									CreateAsset<Scene>("Scene.nrscene");
								}

								if (ImGui::MenuItem("Material"))
								{
									CreateAsset<MaterialAsset>("Material.nrmaterial");
								}

								if (ImGui::MenuItem("Physics Material"))
								{
									CreateAsset<PhysicsMaterial>("PhysicsMaterial.nrpm", 0.6f, 0.6f, 0.0f);
								}

								if (ImGui::MenuItem("Sound Config"))
								{
									CreateAsset<SoundConfig>("SoundConfig.nrsoundc");
								}

								if (ImGui::MenuItem("SOUL Sound"))
								{
									CreateAsset<SOULSound>("New SOUL Sound.soul_sound");
								}

								ImGui::EndMenu();
							}

							if (ImGui::MenuItem("Import"))
							{
								std::string filepath = Application::Get().OpenFile();
								if (!filepath.empty()) 
								{
									FileSystem::CopyFile(filepath, Project::GetAssetDirectory() / mCurrentDirectory->FilePath);
									Refresh();
								}
							}

							if (ImGui::MenuItem("Refresh"))
							{
								Refresh();
							}

							if (ImGui::MenuItem("Copy", "Ctrl+C", nullptr, mSelectionStack.SelectionCount() > 0))
							{
								mCopiedAssets.CopyFrom(mSelectionStack);
							}							
							
							if (ImGui::MenuItem("Paste", "Ctrl+V", nullptr, mCopiedAssets.SelectionCount() > 0))
							{
								PasteCopiedAssets();
							}

							if (ImGui::MenuItem("Duplicate", "Ctrl+D", nullptr, mSelectionStack.SelectionCount() > 0))
							{
								mCopiedAssets.CopyFrom(mSelectionStack);
								PasteCopiedAssets();
							}

							ImGui::Separator();

							if (ImGui::MenuItem("Show in Explorer"))
							{
								FileSystem::OpenDirectoryInExplorer(Project::GetAssetDirectory() / mCurrentDirectory->FilePath);
							}

							ImGui::EndPopup();
						}
						ImGui::PopStyleVar(); // ItemSpacing

						const float paddingForOutline = 2.0f;
						const float scrollBarrOffset = 20.0f + ImGui::GetStyle().ScrollbarSize;
						float panelWidth = ImGui::GetContentRegionAvail().x - scrollBarrOffset;

						float cellSize = sThumbnailSize + sPadding + paddingForOutline;
						int columnCount = (int)(panelWidth / cellSize);
						if (columnCount < 1)
						{
							columnCount = 1;
						}

						{
							const float rowSpacing = 12.0f;
							UI::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(paddingForOutline, rowSpacing));
							ImGui::Columns(columnCount, 0, false);

							UI::ScopedStyle border(ImGuiStyleVar_FrameBorderSize, 0.0f);
							UI::ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
							RenderItems();
						}

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

				if (ImGui::BeginDragDropTarget())
				{
					auto data = ImGui::AcceptDragDropPayload("scene_entity_hierarchy");
					if (data)
					{
						Entity& e = *(Entity*)data->Data;
						Ref<Prefab> prefab = CreateAsset<Prefab>("New Prefab.nrprefab");
						prefab->Create(e);
					}
					ImGui::EndDragDropTarget();
				}

				ImGui::EndTable();
			}
			UI::PopID();
		}
		ImGui::End();
	}

	void ContentBrowserPanel::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& event) { return OnKeyPressedEvent(event); });
	}

	void ContentBrowserPanel::ProjectChanged(const Ref<Project>& project)
	{
		mDirectories.clear();
		mCurrentItems.Clear();
		mBaseDirectory = nullptr;
		mCurrentDirectory = nullptr;
		mNextDirectory = nullptr;
		mPreviousDirectory = nullptr;
		mSelectionStack.Clear();
		mBreadCrumbData.clear();

		mProject = project;

		AssetHandle baseDirectoryHandle = ProcessDirectory(project->GetAssetDirectory().string(), nullptr);
		mBaseDirectory = mDirectories[baseDirectoryHandle];
		ChangeDirectory(mBaseDirectory);

		memset(mSearchBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
	}

	bool ContentBrowserPanel::OnKeyPressedEvent(KeyPressedEvent& e)
	{
		if (!mIsContentBrowserFocused)
		{
			return false;
		}

		bool handled = false;
		if (Input::IsKeyPressed(KeyCode::LeftControl))
		{
			switch (e.GetKeyCode())
			{
			case KeyCode::C:
			{
				mCopiedAssets.CopyFrom(mSelectionStack);
				handled = true;
				break;
			}
			case KeyCode::V:
			{
				PasteCopiedAssets();
				handled = true;
				break;
			}
			case KeyCode::D:
			{
				mCopiedAssets.CopyFrom(mSelectionStack);
				PasteCopiedAssets();
				handled = true;
				break;
			}
			}
		}
		return handled;
	}

	void ContentBrowserPanel::PasteCopiedAssets()
	{
		if (mCopiedAssets.SelectionCount() == 0)
		{
			return;
		}

		auto GetUniquePath = [](const std::filesystem::path& fp)
			{
				int counter = 0;
				auto checkFileName = [&counter, &fp](auto checkFileName) -> std::filesystem::path
					{
						++counter;
						const std::string counterStr = [&counter] {
							if (counter < 10)
							{
								return "0" + std::to_string(counter);
							}
							else
							{
								return std::to_string(counter);
							}
							}();

						std::string basePath = Utils::RemoveExtension(fp.string()) + "_" + counterStr + fp.extension().string();
						if (std::filesystem::exists(basePath))
						{
							return checkFileName(checkFileName);
						}
						else
						{
							return std::filesystem::path(basePath);
						}
					};

				return checkFileName(checkFileName);
			};

		FileSystem::SkipNextFileSystemChange();
		for (AssetHandle copiedAsset : mCopiedAssets)
		{
			const auto& item = mCurrentItems[mCurrentItems.FindItem(copiedAsset)];
			auto originalFilePath = Project::GetAssetDirectory();

			if (item->GetType() == ContentBrowserItem::ItemType::Asset)
			{
				originalFilePath /= item.As<ContentBrowserAsset>()->GetAssetInfo().FilePath;
				auto filepath = GetUniquePath(originalFilePath);
				NR_CORE_VERIFY(!std::filesystem::exists(filepath));
				std::filesystem::copy_file(originalFilePath, filepath);
			}
			else
			{
				originalFilePath /= item.As<ContentBrowserDirectory>()->GetDirectoryInfo()->FilePath;
				auto filepath = GetUniquePath(originalFilePath);
				NR_CORE_VERIFY(!std::filesystem::exists(filepath));
				std::filesystem::copy(originalFilePath, filepath, std::filesystem::copy_options::recursive);
			}
		}

		Refresh();
		mSelectionStack.Clear();
		mCopiedAssets.Clear();
	}

	Ref<DirectoryInfo> ContentBrowserPanel::GetDirectory(const std::filesystem::path& filepath) const
	{
		if (filepath.string() == "" || filepath.string() == ".")
		{
			return mBaseDirectory;
		}

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
		static bool TreeNode(const std::string& id, const std::string& label, ImGuiTreeNodeFlags flags = 0, const Ref<Texture2D>& icon = nullptr)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			if (window->SkipItems)
			{
				return false;
			}

			return ImGui::TreeNodeWithIcon(icon, window->GetID(id.c_str()), flags, label.c_str(), NULL);
		}
	}

	void ContentBrowserPanel::RenderDirectoryHierarchy(Ref<DirectoryInfo>& directory)
	{
		std::string name = directory->FilePath.filename().string();
		std::string id = name + "_TreeNode";
		bool previousState = ImGui::TreeNodeBehaviorIsOpen(ImGui::GetID(id.c_str()));

		// ImGui item height hack
		auto* window = ImGui::GetCurrentWindow();
		window->DC.CurrLineSize.y = 20.0f;
		window->DC.CurrLineTextBaseOffset = 3.0f;

		//---------------------------------------------

		const ImRect itemRect = { window->WorkRect.Min.x, window->DC.CursorPos.y,
								  window->WorkRect.Max.x, window->DC.CursorPos.y + window->DC.CurrLineSize.y };
		
		const bool isItemClicked = [&itemRect, &id]
			{
				if (ImGui::ItemHoverable(itemRect, ImGui::GetID(id.c_str())))
				{
					return ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Left);
				}
				return false;
			}();

		const bool isWindowFocused = ImGui::IsWindowFocused();

		auto fillWithColor = [&](const ImColor& color)
			{
				const ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(color);
				ImGui::GetWindowDrawList()->AddRectFilled(itemRect.Min, itemRect.Max, bgColor);
			};

		// Fill with light selection color if any of the child entities selected
		auto checkIfAnyDescendantSelected = [&](Ref<DirectoryInfo>& directory, auto isAnyDescendantSelected) -> bool
			{
				if (directory->Handle == mCurrentDirectory->Handle)
				{
					return true;
				}

				if (!directory->SubDirectories.empty())
				{
					for (auto& [childHandle, childDir] : directory->SubDirectories)
					{
						if (isAnyDescendantSelected(childDir, isAnyDescendantSelected))
						{
							return true;
						}
					}
				}

				return false;
			};
		const bool isAnyDescendantSelected = checkIfAnyDescendantSelected(directory, checkIfAnyDescendantSelected);
		const bool isActiveDirectory = directory->Handle == mCurrentDirectory->Handle;
		ImGuiTreeNodeFlags flags = (isActiveDirectory ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_SpanFullWidth;

		// Fill background
		//----------------
		if (isActiveDirectory || isItemClicked)
		{
			if (isWindowFocused)
			{
				fillWithColor(Colors::Theme::selection);
			}
			else
			{
				const ImColor col = UI::ColorWithMultipliedValue(Colors::Theme::selection, 0.8f);
				fillWithColor(UI::ColorWithMultipliedSaturation(col, 0.7f));
			}
			ImGui::PushStyleColor(ImGuiCol_Text, Colors::Theme::backgroundDark);
		}
		else if (isAnyDescendantSelected)
		{
			fillWithColor(Colors::Theme::selectionMuted);
		}

		// Tree Node
		//----------
		bool open = UI::TreeNode(id, name, flags, mFolderIcon);

		if (isActiveDirectory || isItemClicked)
		{
			ImGui::PopStyleColor();
		}

		// Fixing slight overlap
		UI::ShiftCursorY(3.0f);
		
		// Draw children
		//--------------
		if (open)
		{
			std::vector<Ref<DirectoryInfo>> directories;
			directories.reserve(mBaseDirectory->SubDirectories.size());
			for (auto& [handle, directory] : directory->SubDirectories)
			{
				directories.emplace_back(directory);
			}
			
			std::sort(directories.begin(), directories.end(), [](const auto& a, const auto& b)
				{
					return a->FilePath.stem().string() < b->FilePath.stem().string();
				});

			for (auto& child : directories)
			{
				RenderDirectoryHierarchy(child);
			}
		}

		UpdateDropArea(directory);

		if (open != previousState && !isActiveDirectory)
		{
			if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.01f))
			{
				ChangeDirectory(directory);
			}
		}

		if (open)
		{
			ImGui::TreePop();
		}
	}

	void ContentBrowserPanel::RenderTopBar(float height)
	{
		ImGui::BeginChild("##top_bar", ImVec2(0, height));
		ImGui::BeginHorizontal("##top_bar", ImGui::GetWindowSize());
		{
			const float edgeOffset = 4.0f;
	
			// Navigation buttons
			{
				UI::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(2.0f, 0.0f));

				auto contenBrowserButton = [height](const char* labelId, const Ref<Texture2D>& icon)
					{
						const ImU32 buttonCol = Colors::Theme::backgroundDark;
						const ImU32 buttonColP = UI::ColorWithMultipliedValue(Colors::Theme::backgroundDark, 0.8f);
						
						UI::ScopedColorStack buttonColors(ImGuiCol_Button, buttonCol,
							ImGuiCol_ButtonHovered, buttonCol,
							ImGuiCol_ButtonActive, buttonColP);
						
						const float iconSize = std::min(24.0f, height);
						const float iconPadding = 3.0f;
						const bool clicked = ImGui::Button(labelId, ImVec2(iconSize, iconSize));
						
						UI::DrawButtonImage(icon, Colors::Theme::textDarker,
							UI::ColorWithMultipliedValue(Colors::Theme::textDarker, 1.2f),
							UI::ColorWithMultipliedValue(Colors::Theme::textDarker, 0.8f),
							UI::RectExpanded(UI::GetItemRect(), -iconPadding, -iconPadding));

						return clicked;
					};

				if (contenBrowserButton("##back", mBackbtnTex))
				{
					mNextDirectory = mCurrentDirectory;
					mPreviousDirectory = mCurrentDirectory->Parent;
					ChangeDirectory(mPreviousDirectory);
				}

				UI::SetTooltip("Previous directory");

				ImGui::Spring(-1.0f, edgeOffset);

				if (contenBrowserButton("##forward", mFwrdbtnTex))
				{
					ChangeDirectory(mNextDirectory);
				}

				UI::SetTooltip("Next directory");

				ImGui::Spring(-1.0f, edgeOffset * 2.0f);

				if (contenBrowserButton("##refresh", mRefreshIcon))
				{
					Refresh();
				}

				UI::SetTooltip("Refresh");

				ImGui::Spring(-1.0f, edgeOffset * 2.0f);
			}

			// Search
			{
				UI::ShiftCursorY(2.0f);
				if (UI::Widgets::SearchWidget<MAX_INPUT_BUFFER_LENGTH>(mSearchBuffer))
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

				UI::ShiftCursorY(-2.0f);
			}

			if (mUpdateNavigationPath)
			{
				mBreadCrumbData.clear();

				Ref<DirectoryInfo> current = mCurrentDirectory;
				while (current && current->Parent != nullptr)
				{
					mBreadCrumbData.push_back(current);
					current = current->Parent;
				}

				std::reverse(mBreadCrumbData.begin(), mBreadCrumbData.end());

				mUpdateNavigationPath = false;
			}

			// Breadcrumbs
			{
				UI::ScopedFont boldFont(ImGui::GetIO().Fonts->Fonts[0]);
				UI::ScopedColor textColor(ImGuiCol_Text, Colors::Theme::textDarker);
			
				const std::string& assetsDirectoryName = mProject->GetConfig().AssetDirectory;
				ImVec2 textSize = ImGui::CalcTextSize(assetsDirectoryName.c_str());
				const float textPadding = ImGui::GetStyle().FramePadding.y;

				if (ImGui::Selectable(assetsDirectoryName.c_str(), false, 0, ImVec2(textSize.x, textSize.y + textPadding)))
				{
					ChangeDirectory(mBaseDirectory);
				}

				UpdateDropArea(mBaseDirectory);
				
				for (auto& directory : mBreadCrumbData)
				{
					ImGui::Text("/");
					std::string directoryName = directory->FilePath.filename().string();
					ImVec2 textSize = ImGui::CalcTextSize(directoryName.c_str());
					
					if (ImGui::Selectable(directoryName.c_str(), false, 0, ImVec2(textSize.x, textSize.y + textPadding)))
					{
						ChangeDirectory(directory);
					}

					UpdateDropArea(directory);
				}
			}

			// Settings button
			ImGui::Spring();
			if (UI::Widgets::OptionsButton())
			{
				ImGui::OpenPopup("ContentBrowserSettings");
			}
			UI::SetTooltip("Content Browser settings");

			if (UI::BeginPopup("ContentBrowserSettings"))
			{
				if (ImGui::MenuItem("Show asset type", nullptr, &mShowAssetType))
				{
				}

				ImGui::SliderFloat("##thumbnail_size", &sThumbnailSize, 96.0f, 512.0f, "%.0f");
				UI::SetTooltip("Thumnail Size");

				UI::EndPopup();
			}

		}
		ImGui::EndHorizontal();
		ImGui::EndChild();
	}

	static std::mutex sLockMutex;
	void ContentBrowserPanel::RenderItems()
	{
		mIsAnyItemHovered = false;

		bool openDeleteDialogue = false;

		std::scoped_lock<std::mutex> lock(sLockMutex);
		for (auto& item : mCurrentItems)
		{
			item->RenderBegin();
			CBItemActionResult result = item->Render(sThumbnailSize, mShowAssetType);

			if (result.IsSet(ContentBrowserAction::ClearSelections))
			{
				ClearSelections();
			}

			if (result.IsSet(ContentBrowserAction::Selected) && !mSelectionStack.IsSelected(item->GetID()))
			{
				mSelectionStack.Select(item->GetID());
				item->SetSelected(true);
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
					toSelect->SetSelected(true);
					mSelectionStack.Select(toSelect->GetID());
				}
			}

			if (result.IsSet(ContentBrowserAction::StartRenaming))
			{
				item->StartRenaming();
			}

			if (result.IsSet(ContentBrowserAction::Copy))
			{
				mCopiedAssets.Select(item->GetID());
			}

			if (result.IsSet(ContentBrowserAction::Reload))
			{
				AssetManager::ReloadData(item->GetID());
			}
			if (result.IsSet(ContentBrowserAction::OpenDeleteDialogue))
			{
				openDeleteDialogue = true;
			}
			if (result.IsSet(ContentBrowserAction::ShowInExplorer))
			{
				if (item->GetType() == ContentBrowserItem::ItemType::Directory)
				{
					FileSystem::ShowFileInExplorer(mProject->GetAssetDirectory() / mCurrentDirectory->FilePath / item->GetName());
				}
				else
				{
					FileSystem::ShowFileInExplorer(AssetManager::GetFileSystemPath(AssetManager::GetMetadata(item->GetID())));
				}
			}
			if (result.IsSet(ContentBrowserAction::OpenExternal))
			{
				if (item->GetType() == ContentBrowserItem::ItemType::Directory)
				{
					FileSystem::OpenExternally(mCurrentDirectory->FilePath / item->GetName());
				}
				else
				{
					FileSystem::OpenExternally(AssetManager::GetFileSystemPath(AssetManager::GetMetadata(item->GetID())));
				}
			}
			if (result.IsSet(ContentBrowserAction::Hovered))
			{
				mIsAnyItemHovered = true;
			}

			item->RenderEnd();

			if (result.IsSet(ContentBrowserAction::Renamed))
			{
				RefreshWithoutLock();
				SortItemList();
				item->SetSelected(true);
				mSelectionStack.Select(item->GetID());
				break;
			}

			if (result.IsSet(ContentBrowserAction::NavigateToThis))
			{
				ChangeDirectory(item.As<ContentBrowserDirectory>()->GetDirectoryInfo());
				break;
			}

			if (result.IsSet(ContentBrowserAction::Refresh))
			{
				RefreshWithoutLock();
				break;
			}
		}

		if (openDeleteDialogue)
		{
			ImGui::OpenPopup("Delete");
			openDeleteDialogue = false;
		}
	}

	void ContentBrowserPanel::Refresh()
	{
		std::lock_guard<std::mutex> lock(sLockMutex);
		RefreshWithoutLock();
	}

	void ContentBrowserPanel::RefreshWithoutLock()
	{

		mCurrentItems.Clear();
		mDirectories.clear();

		Ref<DirectoryInfo> currentDirectory = mCurrentDirectory;
		AssetHandle baseDirectoryHandle = ProcessDirectory(mProject->GetAssetDirectory().string(), nullptr);
		mBaseDirectory = mDirectories[baseDirectoryHandle];
		mCurrentDirectory = GetDirectory(currentDirectory->FilePath);

		if (!mCurrentDirectory)
		{
			mCurrentDirectory = mBaseDirectory;
		}

		ChangeDirectory(mCurrentDirectory);
	}

	void ContentBrowserPanel::UpdateInput()
	{
		if (!mIsContentBrowserHovered)
		{
			return;
		}

		if ((!mIsAnyItemHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left)) || Input::IsKeyPressed(KeyCode::Escape))
		{
			ClearSelections();
		}

		if (Input::IsKeyPressed(KeyCode::Delete) && mSelectionStack.SelectionCount() > 0)
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
		for (auto& item : mCurrentItems)
		{
			item->SetSelected(false);
			if (item->IsRenaming())
			{
				item->StopRenaming();
			}
		}

		mSelectionStack.Clear();
	}

	void ContentBrowserPanel::RenderDeleteDialogue()
	{
		if (ImGui::BeginPopupModal("Delete", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (mSelectionStack.SelectionCount() == 0)
			{
				ImGui::CloseCurrentPopup();
			}

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

	void ContentBrowserPanel::RemoveDirectory(Ref<DirectoryInfo>& directory, bool removeFromParent)
	{
		if (directory->Parent && removeFromParent)
		{
			auto& childList = directory->Parent->SubDirectories;
			childList.erase(childList.find(directory->Handle));
		}

		for (auto& [handle, subdir] : directory->SubDirectories)
		{
			RemoveDirectory(subdir, false);
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
			std::string subdirName = subdir->FilePath.filename().string();
			if (subdirName.find(queryLowerCase) != std::string::npos)
			{
				results.Items.push_back(Ref<ContentBrowserDirectory>::Create(subdir, mFolderIcon));
			}

			ContentBrowserItemList list = Search(query, subdir);
			results.Items.insert(results.Items.end(), list.Items.begin(), list.Items.end());
		}

		for (auto& assetHandle : directoryInfo->Assets)
		{
			auto& asset = AssetManager::GetMetadata(assetHandle);
			std::string filename = Utils::ToLower(asset.FilePath.filename().string());

			if (filename.find(queryLowerCase) != std::string::npos)
			{
				results.Items.push_back(Ref<ContentBrowserAsset>::Create(asset, mAssetIconMap.find(asset.FilePath.extension().string()) != mAssetIconMap.end() ? mAssetIconMap[asset.FilePath.extension().string()] : mFileTex));
			}

			if (queryLowerCase[0] != '.')
			{
				continue;
			}

			if (asset.FilePath.extension().string().find(std::string(&queryLowerCase[1])) != std::string::npos)
			{
				results.Items.push_back(Ref<ContentBrowserAsset>::Create(asset, mAssetIconMap.find(asset.FilePath.extension().string()) != mAssetIconMap.end() ? mAssetIconMap[asset.FilePath.extension().string()] : mFileTex));
			}
		}

		return results;
	}

	void ContentBrowserPanel::FileSystemChanged(const std::vector<FileSystemChangedEvent>& e)
	{
		Refresh();
	}

	ContentBrowserPanel* ContentBrowserPanel::sInstance;
}