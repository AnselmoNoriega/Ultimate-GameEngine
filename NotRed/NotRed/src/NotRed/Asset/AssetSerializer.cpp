#include "nrpch.h"
#include "AssetSerializer.h"

#include "NotRed/Util/StringUtils.h"
#include "NotRed/Util/FileSystem.h"
#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Renderer/Renderer.h"
#include "NotRed/Audio/AudioFileUtils.h"

#include "AssetManager.h"

#include "yaml-cpp/yaml.h"

namespace NR
{
	bool TextureSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		asset = Texture2D::Create(AssetManager::GetFileSystemPath(metadata));
		asset->Handle = metadata.Handle;

		bool result = asset.As<Texture2D>()->Loaded();
		if (!result)
		{
			asset->ModifyFlags(AssetFlag::Invalid, true);
		}
		return result;
	}

	bool MeshAssetSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		Ref<Asset> temp = asset;
		asset = Ref<MeshAsset>::Create(AssetManager::GetFileSystemPath(metadata));
		asset->Handle = metadata.Handle;
		return (asset.As<MeshAsset>())->GetVertices().size() > 0;
	}

	bool EnvironmentSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		auto [radiance, irradiance] = Renderer::CreateEnvironmentMap(AssetManager::GetFileSystemPath(metadata));

		if (!radiance || !irradiance)
		{
			return false;
		}

		asset = Ref<Environment>::Create(radiance, irradiance);
		asset->Handle = metadata.Handle;
		return true;
	}

	void PhysicsMaterialSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<PhysicsMaterial> material = asset.As<PhysicsMaterial>();

		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "StaticFriction" << material->StaticFriction;
		out << YAML::Key << "DynamicFriction" << material->DynamicFriction;
		out << YAML::Key << "Bounciness" << material->Bounciness;
		out << YAML::EndMap;

		std::ofstream fout(metadata.FilePath);
		fout << out.c_str();
	}

	bool PhysicsMaterialSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		std::ifstream stream(AssetManager::GetFileSystemPath(metadata));
		if (!stream.is_open())
		{
			return false;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());

		float staticFriction = data["StaticFriction"].as<float>();
		float dynamicFriction = data["DynamicFriction"].as<float>();
		float bounciness = data["Bounciness"].as<float>();

		asset = Ref<PhysicsMaterial>::Create(staticFriction, dynamicFriction, bounciness);
		asset->Handle = metadata.Handle;
		return true;
	}

	bool AudioFileSourceSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		if (auto opt = AudioFileUtils::GetFileInfo(metadata))
		{
			AudioFileUtils::AudioFileInfo info = opt.value();
			asset = Ref<AudioFile>::Create(info.Duration, info.SamplingRate, info.BidDepth, info.NumChannels, info.FileSize);
		}
		else
		{
			asset = Ref<AudioFile>::Create();
		}
		asset->Handle = metadata.Handle;
		return true;
	}
}