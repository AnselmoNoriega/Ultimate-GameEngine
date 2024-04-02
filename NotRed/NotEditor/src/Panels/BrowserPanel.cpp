#include "BrowserPanel.h"

#include <imgui/imgui.h>

namespace NR
{
	static const std::filesystem::path sAssetsPath = "Assets";

	BrowserPanel::BrowserPanel()
		: mCurrentDirectory(sAssetsPath)
	{
	}

	void BrowserPanel::ImGuiRender()
	{
		ImGui::Begin("Content Browser");

		if (mCurrentDirectory.string() != sAssetsPath)
		{
			if (ImGui::Button("Back"))
			{
				mCurrentDirectory = mCurrentDirectory.parent_path();
			}
		}

		for (auto& entry : std::filesystem::directory_iterator(mCurrentDirectory))
		{
			auto relativePath = std::filesystem::relative(entry.path(), sAssetsPath);
			std::string entryPath = relativePath.filename().string();

			if (entry.is_directory())
			{
				if (ImGui::Button(entryPath.c_str()))
				{
					mCurrentDirectory /= relativePath.filename();
				}
			}
			else
			{
				if (ImGui::Button(entryPath.c_str()))
				{

				}
			}
		}

		ImGui::End();
	}
}