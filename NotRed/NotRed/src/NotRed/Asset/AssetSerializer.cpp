#include "nrpch.h"
#include "AssetSerializer.h"

#include "NotRed/Util/StringUtils.h"
#include "NotRed/Util/FileSystem.h"
#include "NotRed/Renderer/Mesh.h"

#include "yaml-cpp/yaml.h"

namespace NR
{
	void AssetSerializer::SerializeAsset(const Ref<Asset>& asset, AssetType type)
	{
		YAML::Emitter out;

		out << YAML::BeginMap;

		switch (type)
		{
		case AssetType::PhysicsMat:
		{
			Ref<PhysicsMaterial> material = Ref<PhysicsMaterial>(asset);
			out << YAML::Key << "StaticFriction" << material->StaticFriction;
			out << YAML::Key << "DynamicFriction" << material->DynamicFriction;
			out << YAML::Key << "Bounciness" << material->Bounciness;
			break;
		}
		}
		out << YAML::EndMap;

		std::ofstream fout(asset->FilePath);
		fout << out.c_str();
	}

	Ref<Asset> AssetSerializer::Deserialize(const std::string& filepath, int parentIndex, bool reimport, AssetType type)
	{
		Ref<Asset> asset = Ref<Asset>::Create();

		std::string extension = Utils::GetExtension(filepath);

		bool loadYAMLData = true;

		switch (type)
		{
		case AssetType::Mesh:
		{
			if (extension != "blend")
				asset = Ref<Mesh>::Create(filepath);
			loadYAMLData = false;
			break;
		}
		case AssetType::Texture:
		{
			asset = Texture2D::Create(filepath);
			loadYAMLData = false;
			break;
		}
		case AssetType::EnvMap:
		{
			loadYAMLData = false;
			break;
		}
		case AssetType::Scene:
		case AssetType::Audio:
		case AssetType::Script:
		case AssetType::Other:
		{
			loadYAMLData = false;
			break;
		}
		}

		if (loadYAMLData)
		{
			asset = DeserializeYAML(filepath, type);
			NR_CORE_ASSERT(asset, "Failed to load asset");
		}

		bool hasMeta = FileSystem::Exists(filepath + ".meta");
		if (hasMeta)
		{
			asset->FilePath = filepath;
			LoadMetaData(asset);
			std::replace(asset->FilePath.begin(), asset->FilePath.end(), '\\', '/');
		}
		else
		{
			asset->FilePath = filepath;
			std::replace(asset->FilePath.begin(), asset->FilePath.end(), '\\', '/');
			asset->Handle = std::hash<std::string>()(asset->FilePath);
			asset->FileName = Utils::RemoveExtension(Utils::GetFilename(filepath));
			asset->Extension = Utils::GetExtension(filepath);
			asset->ParentDirectory = parentIndex;
			asset->Type = type;
		}

		if (!hasMeta || reimport)
		{
			CreateMetaFile(asset);
		}

		return asset;
	}

	Ref<Asset> AssetSerializer::DeserializeYAML(const std::string& filepath, AssetType type)
	{
		std::ifstream stream(filepath);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());

		if (type == AssetType::PhysicsMat)
		{
			float staticFriction = data["StaticFriction"].as<float>();
			float dynamicFriction = data["DynamicFriction"].as<float>();
			float bounciness = data["Bounciness"].as<float>();

			return Ref<PhysicsMaterial>::Create(staticFriction, dynamicFriction, bounciness);
		}

		return nullptr;
	}

	void AssetSerializer::LoadMetaData(Ref<Asset>& asset)
	{
		std::ifstream stream(asset->FilePath + ".meta");
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Asset"])
		{
			NR_CORE_ASSERT("Invalid File Format");
		}

		asset->Handle = data["Asset"].as<AssetHandle>();
		asset->FileName = data["FileName"].as<std::string>();
		asset->FilePath = data["FilePath"].as<std::string>();
		asset->Extension = data["Extension"].as<std::string>();
		asset->ParentDirectory = data["Directory"].as<int>();
		asset->Type = (AssetType)data["Type"].as<int>();
	}

	void AssetSerializer::CreateMetaFile(const Ref<Asset>& asset)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Asset" << YAML::Value << asset->Handle;
		out << YAML::Key << "FileName" << YAML::Value << asset->FileName;
		out << YAML::Key << "FilePath" << YAML::Value << asset->FilePath;
		out << YAML::Key << "Extension" << YAML::Value << asset->Extension;
		out << YAML::Key << "Directory" << YAML::Value << asset->ParentDirectory;
		out << YAML::Key << "Type" << YAML::Value << (int)asset->Type;
		out << YAML::EndMap;

		std::ofstream fout(asset->FilePath + ".meta");
		fout << out.c_str();
	}

}