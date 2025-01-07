#pragma once

#include <filesystem>

#include "NotRed/Core/Ref.h"

namespace NR
{
	struct ProjectConfig
	{
		std::string Name;

		std::string AssetDirectory;
		std::string AssetRegistryPath;

		std::string AudioCommandsRegistryPath = "Assets/AudioCommandsRegistry.nrr";

		std::string MeshPath;
		std::string MeshSourcePath;

		std::string ScriptModulePath;
		std::string DefaultNamespace;

		std::string StartScene;

		bool ReloadAssemblyOnPlay;

		std::string ProjectFileName;
		std::string ProjectDirectory;
	};

	class Project : public RefCounted
	{
	public:
		Project();
		~Project();

		const ProjectConfig& GetConfig() const { return mConfig; }
		static Ref<Project> GetActive() { return sActiveProject; }

		static void SetActive(Ref<Project> project);

		static const std::string& GetProjectName()
		{
			NR_CORE_ASSERT(sActiveProject);
			return sActiveProject->GetConfig().Name;
		}

		static std::filesystem::path GetProjectDirectory()
		{
			NR_CORE_ASSERT(sActiveProject);
			return sActiveProject->GetConfig().ProjectDirectory;
		}

		static std::filesystem::path GetAssetDirectory()
		{
			NR_CORE_ASSERT(sActiveProject);
			return std::filesystem::path(sActiveProject->GetConfig().ProjectDirectory) / sActiveProject->GetConfig().AssetDirectory;
		}

		static std::filesystem::path GetAssetRegistryPath()
		{
			NR_CORE_ASSERT(sActiveProject);
			return std::filesystem::path(sActiveProject->GetConfig().ProjectDirectory) / sActiveProject->GetConfig().AssetRegistryPath;
		}

		static std::filesystem::path GetAudioCommandsRegistryPath()
		{
			NR_CORE_ASSERT(sActiveProject);
			return std::filesystem::path(sActiveProject->GetConfig().ProjectDirectory) / sActiveProject->GetConfig().AudioCommandsRegistryPath;
		}

		static std::filesystem::path GetScriptModulePath()
		{
			NR_CORE_ASSERT(sActiveProject);
			return std::filesystem::path(sActiveProject->GetConfig().ProjectDirectory) / sActiveProject->GetConfig().ScriptModulePath;
		}

		static std::filesystem::path GetMeshPath()
		{
			NR_CORE_ASSERT(sActiveProject);
			return std::filesystem::path(sActiveProject->GetConfig().ProjectDirectory) / sActiveProject->GetConfig().MeshPath;
		}

		static std::filesystem::path GetScriptModuleFilePath()
		{
			NR_CORE_ASSERT(sActiveProject);
			return GetScriptModulePath() / fmt::format("{0}.dll", GetProjectName());
		}

		static std::filesystem::path GetCacheDirectory()
		{
			NR_CORE_ASSERT(sActiveProject);
			return std::filesystem::path(sActiveProject->GetConfig().ProjectDirectory) / "Cache";
		}

	private:
		void Deserialized();

	private:
		ProjectConfig mConfig;

		inline static Ref<Project> sActiveProject;

	private:
		friend class ProjectSettingsWindow;
		friend class ProjectSerializer;
	};
}