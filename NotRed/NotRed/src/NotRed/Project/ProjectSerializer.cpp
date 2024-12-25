#include "nrpch.h"
#include "ProjectSerializer.h"

#include <filesystem>

#include "yaml-cpp/yaml.h"

namespace NR
{
	ProjectSerializer::ProjectSerializer(Ref<Project> project)
		: mProject(project)
	{
	}

	void ProjectSerializer::Serialize(const std::string& filepath)
	{
		NR_CORE_ASSERT(false);
	}

	bool ProjectSerializer::Deserialize(const std::string& filepath)
	{
		std::ifstream stream(filepath);
		NR_CORE_ASSERT(stream);

		std::stringstream strStream;
		strStream << stream.rdbuf();
		
		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Project"])
		{
			return false;
		}

		YAML::Node rootNode = data["Project"];
		if (!rootNode["Name"])
		{
			return false;
		}

		auto& config = mProject->mConfig;
		config.Name = rootNode["Name"].as<std::string>();
		config.AssetDirectory = rootNode["AssetDirectory"].as<std::string>();
		config.AssetRegistryPath = rootNode["AssetRegistry"].as<std::string>();
		config.MeshPath = rootNode["MeshPath"].as<std::string>();
		config.MeshSourcePath = rootNode["MeshSourcePath"].as<std::string>();
		config.ScriptModulePath = rootNode["ScriptModulePath"].as<std::string>();

		if (rootNode["DefaultNamespace"])
		{
			config.DefaultNamespace = rootNode["DefaultNamespace"].as<std::string>();
		}
		else
		{
			config.DefaultNamespace = config.Name;
		}

		config.StartScene = rootNode["StartScene"].as<std::string>();
		std::filesystem::path projectPath = filepath;
		config.ProjectDirectory = projectPath.parent_path().string();
		mProject->Deserialized();

		return true;
	}
}