#pragma once

#include <filesystem>

#include "NotRed/Renderer/Texture.h"

namespace NR
{
	class BrowserPanel
	{
	public:
		BrowserPanel();

		void ImGuiRender();

	private:

		std::filesystem::path mCurrentDirectory;

		Ref<Texture2D> mFolderIcon;
		Ref<Texture2D> mFileIcon;
	};
}