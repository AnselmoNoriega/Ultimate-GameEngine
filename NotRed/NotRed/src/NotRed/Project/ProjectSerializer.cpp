#include "nrpch.h"
#include "ProjectSerializer.h"

#include <filesystem>

#include "NotRed/Physics/PhysicsManager.h"
#include "NotRed/Physics/PhysicsLayer.h"

#include "NotRed/Util/YAMLSerializationHelpers.h"

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


			out << YAML::Key << "Physics" << YAML::Value;
			{
				out << YAML::BeginMap;
				const auto& physicsSettings = PhysicsManager::GetSettings();

				out << YAML::Key << "FixedTimestep" << YAML::Value << physicsSettings.FixedDeltaTime;
				out << YAML::Key << "Gravity" << YAML::Value << physicsSettings.Gravity;
				out << YAML::Key << "BroadphaseAlgorithm" << YAML::Value << (uint32_t)physicsSettings.BroadphaseAlgorithm;

				if (physicsSettings.BroadphaseAlgorithm != BroadphaseType::AutomaticBoxPrune)
				{
					out << YAML::Key << "WorldBoundsMin" << YAML::Value << physicsSettings.WorldBoundsMin;
					out << YAML::Key << "WorldBoundsMax" << YAML::Value << physicsSettings.WorldBoundsMax;
					out << YAML::Key << "WorldBoundsSubdivisions" << YAML::Value << physicsSettings.WorldBoundsSubdivisions;
				}

				out << YAML::Key << "FrictionModel" << YAML::Value << (int)physicsSettings.FrictionModel;
				out << YAML::Key << "SolverPositionIterations" << YAML::Value << physicsSettings.SolverIterations;
				out << YAML::Key << "SolverVelocityIterations" << YAML::Value << physicsSettings.SolverVelocityIterations;

#ifdef NR_DEBUG
				out << YAML::Key << "DebugOnPlay" << YAML::Value << physicsSettings.DebugOnPlay;
				out << YAML::Key << "DebugType" << YAML::Value << (int8_t)physicsSettings.DebugType;
#endif

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

		// Physics
		auto physicsNode = rootNode["Physics"];
		if (physicsNode)
		{
			auto& physicsSettings = PhysicsManager::GetSettings();

			physicsSettings.FixedDeltaTime = physicsNode["FixedTimestep"] ? physicsNode["FixedTimestep"].as<float>() : 1.0f / 100.0f;
			physicsSettings.Gravity = physicsNode["Gravity"] ? physicsNode["Gravity"].as<glm::vec3>() : glm::vec3{ 0.0f, -9.81f, 0.0f };
			physicsSettings.BroadphaseAlgorithm = physicsNode["BroadphaseAlgorithm"] ? (BroadphaseType)physicsNode["BroadphaseAlgorithm"].as<uint32_t>() : BroadphaseType::AutomaticBoxPrune;
			
			if (physicsSettings.BroadphaseAlgorithm != BroadphaseType::AutomaticBoxPrune)
			{
				physicsSettings.WorldBoundsMin = physicsNode["WorldBoundsMin"] ? physicsNode["WorldBoundsMin"].as<glm::vec3>() : glm::vec3(-100.0f);
				physicsSettings.WorldBoundsMax = physicsNode["WorldBoundsMax"] ? physicsNode["WorldBoundsMax"].as<glm::vec3>() : glm::vec3(100.0f);
				physicsSettings.WorldBoundsSubdivisions = physicsNode["WorldBoundsSubdivisions"] ? physicsNode["WorldBoundsSubdivisions"].as<uint32_t>() : 2;
			}

			physicsSettings.FrictionModel = physicsNode["FrictionModel"] ? (FrictionType)physicsNode["FrictionModel"].as<int>() : FrictionType::Patch;
			physicsSettings.SolverIterations = physicsNode["SolverPositionIterations"] ? physicsNode["SolverPositionIterations"].as<uint32_t>() : 8;
			physicsSettings.SolverVelocityIterations = physicsNode["SolverVelocityIterations"] ? physicsNode["SolverVelocityIterations"].as<uint32_t>() : 2;

#ifdef NR_DEBUG
			physicsSettings.DebugOnPlay = physicsNode["DebugOnPlay"] ? physicsNode["DebugOnPlay"].as<bool>() : true;
			physicsSettings.DebugType = physicsNode["DebugType"] ? (DebugType)physicsNode["DebugType"].as<int8_t>() : DebugType::LiveDebug;
#endif
			auto physicsLayers = physicsNode["Layers"];
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
		}

		mProject->Deserialized();

		return true;
	}
}