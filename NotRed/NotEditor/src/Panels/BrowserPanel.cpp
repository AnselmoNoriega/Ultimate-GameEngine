#include "BrowserPanel.h"

#include <imgui/imgui.h>

#include "NotRed/Project/Project.h"

namespace NR
{
	BrowserPanel::BrowserPanel()
		: mBaseDirectory(Project::GetAssetDirectory()), mCurrentDirectory(mBaseDirectory)
	{
		mFolderIcon = Texture2D::Create("Resources/Icons/BrowserImg/folderImg.png");
		mFileIcon = Texture2D::Create("Resources/Icons/BrowserImg/fileImg.png");
	}

	void BrowserPanel::ImGuiRender()
	{
		ImGui::Begin("Content Browser");

		if (mCurrentDirectory.string() != mBaseDirectory)
		{
			if (ImGui::Button("Back"))
			{
				mCurrentDirectory = mCurrentDirectory.parent_path();
			}
		}

		static float padding = 16.0f;
		static float thumbnailSize = 90.0f;
		float cellSize = thumbnailSize + padding;

		float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1)
		{
			columnCount = 1;
		}

		ImGui::Columns(columnCount, 0, false);

		for (auto& entry : std::filesystem::directory_iterator(mCurrentDirectory))
		{
			const auto& path = entry.path();
			std::string fileName = path.filename().string();

			ImGui::PushID(fileName.c_str());

			Ref<Texture2D> icon = entry.is_directory() ? mFolderIcon : mFileIcon;
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			ImGui::ImageButton((ImTextureID)icon->GetRendererID(), { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });
			
			if (ImGui::BeginDragDropSource())
			{
				std::filesystem::path relativePath(path);
				const wchar_t* itemPath = relativePath.c_str();
				ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t));
				ImGui::EndDragDropSource();
			}

			ImGui::PopStyleColor();

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				if (entry.is_directory())
				{
					mCurrentDirectory /= path.filename();
				}
			}

			ImGui::TextWrapped(fileName.c_str());

			ImGui::NextColumn();

			ImGui::PopID();
		}

		ImGui::Columns(1);

		ImGui::End();
	}
}