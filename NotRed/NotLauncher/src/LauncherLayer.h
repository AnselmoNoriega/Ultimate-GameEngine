#pragma once

#include <NotRed.h>
#include <functional>

#include "imgui.h"
#include "NotRed/Project/UserPreferences.h"

namespace NR
{
	struct LauncherProperties
	{
		Ref<UserPreferences> UserPreferences;
		std::string InstallPath;
		std::function<void(std::string)> ProjectOpenedCallback;
	};

	class LauncherLayer : public Layer
	{
	public:
		LauncherLayer(const LauncherProperties& properties);
		~LauncherLayer() override;

		void Attach() override;
		void Detach() override;
		void ImGuiRender() override;

	private:
		void CreateProject(std::filesystem::path projectPath);
		void AddProjectToRecents(const std::filesystem::path& projectFile);

	private:
		LauncherProperties mProperties;
		ImGuiID mHoveredProjectID;
		Ref<Texture2D> mNotRedLogoTexture;
	};
}