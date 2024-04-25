#pragma once

#include <string>
#include <filesystem>

#include "NotRed/Core/Core.h"

namespace NR
{

	struct ProjectConfig
	{
		std::string Name = "Nameless";

		std::filesystem::path StartScene;

		std::filesystem::path AssetDirectory;
		std::filesystem::path ScriptModulePath;
	};

	class Project
	{
	public:
		static Ref<Project> New();
		static Ref<Project> Load(const std::filesystem::path& path);
		static bool SaveAs(const std::filesystem::path& path);

		static const std::filesystem::path& GetProjectDirectory()
		{
			NR_CORE_ASSERT(sCurrentProject, "No project active!");
			return sCurrentProject->mProjectDirectory;
		}

		static std::filesystem::path GetAssetDirectory()
		{
			NR_CORE_ASSERT(sCurrentProject, "No project active!");
			return GetProjectDirectory() / sCurrentProject->mConfig.AssetDirectory;
		}

		static std::filesystem::path GetAssetFileSystemPath(const std::filesystem::path& path)
		{
			NR_CORE_ASSERT(sCurrentProject, "No project active!");
			return GetAssetDirectory() / path;
		}

		ProjectConfig& GetConfig() { return mConfig; }

		static Ref<Project> GetActive() { return sCurrentProject; }

	private:
		ProjectConfig mConfig;
		std::filesystem::path mProjectDirectory;

		inline static Ref<Project> sCurrentProject;
	};

}
