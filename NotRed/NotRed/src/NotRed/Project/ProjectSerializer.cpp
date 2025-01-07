#include "nrpch.h"
#include "ProjectSerializer.h"

#include <filesystem>

#include "NotRed/Physics/PhysicsLayer.h"

#include "yaml-cpp/yaml.h"

namespace NR
{
	ProjectSerializer::ProjectSerializer(Ref<Project> project)
		: mProject(project)
	{
	}

	void ProjectSerializer::Serialize(const std::string& filepath)
	{
		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "Project" << YAML::Value;
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Name" << YAML::Value << mProject->mConfig.Name;
			out << YAML::Key << "AssetDirectory" << YAML::Value << mProject->mConfig.AssetDirectory;
			out << YAML::Key << "AssetRegistry" << YAML::Value << mProject->mConfig.AssetRegistryPath;
			out << YAML::Key << "AudioCommandsRegistryPath" << YAML::Value << mProject->mConfig.AudioCommandsRegistryPath;
			out << YAML::Key << "MeshPath" << YAML::Value << mProject->mConfig.MeshPath;
			out << YAML::Key << "MeshSourcePath" << YAML::Value << mProject->mConfig.MeshSourcePath;
			out << YAML::Key << "ScriptModulePath" << YAML::Value << mProject->mConfig.ScriptModulePath;
			out << YAML::Key << "StartScene" << YAML::Value << mProject->mConfig.StartScene;
			out << YAML::Key << "ReloadAssemblyOnPlay" << YAML::Value << mProject->mConfig.ReloadAssemblyOnPlay;

			// > 1 because of the Default layer
			if (PhysicsLayerManager::GetLayerCount() > 1)
			{
				out << YAML::Key << "PhysicsLayers";
				out << YAML::Value << YAML::BeginSeq;

				for (const auto& layer : PhysicsLayerManager::GetLayers())
				{
					// Never serialize the Default layer
					if (layer.ID == 0)
					{
						continue;
					}

					out << YAML::BeginMap;
					out << YAML::Key << "Name" << YAML::Value << layer.Name;
					out << YAML::Key << "CollidesWith" << YAML::Value;
					out << YAML::BeginSeq;

					for (const auto& collidingLayer : PhysicsLayerManager::GetLayerCollisions(layer.ID))
					{
						out << YAML::BeginMap;
						out << YAML::Key << "Name" << YAML::Value << collidingLayer.Name;
						out << YAML::EndMap;
					}

					out << YAML::EndSeq;
					out << YAML::EndMap;
				}
				out << YAML::EndSeq;
			}
			out << YAML::EndMap;
		}

		out << YAML::EndMap;
		std::ofstream fout(filepath);
		fout << out.c_str();
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
		if (rootNode["AudioCommandsRegistryPath"])
		{
			config.AudioCommandsRegistryPath = rootNode["AudioCommandsRegistryPath"].as<std::string>();
		}
		config.MeshPath = rootNode["MeshPath"].as<std::string>();
		config.MeshSourcePath = rootNode["MeshSourcePath"].as<std::string>();
		config.ScriptModulePath = rootNode["ScriptModulePath"].as<std::string>();

		config.DefaultNamespace = rootNode["DefaultNamespace"] ? rootNode["DefaultNamespace"].as<std::string>() : config.Name;
		config.StartScene = rootNode["StartScene"] ? rootNode["StartScene"].as<std::string>() : "";
		config.ReloadAssemblyOnPlay = rootNode["ReloadAssemblyOnPlay"] ? rootNode["ReloadAssemblyOnPlay"].as<bool>() : true;

		std::filesystem::path projectPath = filepath;
		config.ProjectFileName = projectPath.filename().string();
		config.ProjectDirectory = projectPath.parent_path().string();

		auto physicsLayers = rootNode["PhysicsLayers"];
		if (physicsLayers)
		{
			for (auto layer : physicsLayers)
			{
				PhysicsLayerManager::AddLayer(layer["Name"].as<std::string>(), false);
			}

			for (auto layer : physicsLayers)
			{
				const PhysicsLayer& layerInfo = PhysicsLayerManager::GetLayer(layer["Name"].as<std::string>());
				auto collidesWith = layer["CollidesWith"];
				if (collidesWith)
				{
					for (auto collisionLayer : collidesWith)
					{
						const auto& otherLayer = PhysicsLayerManager::GetLayer(collisionLayer["Name"].as<std::string>());
						PhysicsLayerManager::SetLayerCollision(layerInfo.ID, otherLayer.ID, true);
					}
				}
			}
		}

		mProject->Deserialized();

		return true;
	}
}