#pragma once

#include <filesystem>

namespace NR
{
	class BrowserPanel
	{
	public:
		BrowserPanel();

		void ImGuiRender();

	private:

		std::filesystem::path mCurrentDirectory;
	};
}