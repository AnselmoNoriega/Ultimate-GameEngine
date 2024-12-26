#include "nrpch.h"
#include "AssetSerializer.h"

#include "NotRed/Util/StringUtils.h"
#include "NotRed/Util/FileSystem.h"

#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Renderer/Renderer.h"
#include "NotRed/Renderer/MaterialAsset.h"

#include "NotRed/Audio/AudioFileUtils.h"
#include "NotRed/Audio/Sound.h"

#include "AssetManager.h"

#include "yaml-cpp/yaml.h"
#include "NotRed/Util/YAMLSerializationHelpers.h"

namespace NR
{
	bool TextureSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		asset = Texture2D::Create(AssetManager::GetFileSystemPathString(metadata));
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
		asset = Ref<MeshAsset>::Create(AssetManager::GetFileSystemPathString(metadata));
		asset->Handle = metadata.Handle;
		return (asset.As<MeshAsset>())->GetStaticVertices().size() > 0;
	}

	void MaterialAssetSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
#define NR_SERIALIZE_PROPERTY(propName, propVal) out << YAML::Key << #propName << YAML::Value << propVal
		Ref<MaterialAsset> material = asset.As<MaterialAsset>();
		
		YAML::Emitter out;
		out << YAML::BeginMap; // Material
		out << YAML::Key << "Material" << YAML::Value;
		{
			out << YAML::BeginMap;

			NR_SERIALIZE_PROPERTY(AlbedoColor, material->GetAlbedoColor());
			NR_SERIALIZE_PROPERTY(Emission, material->GetEmission());
			NR_SERIALIZE_PROPERTY(UseNormalMap, material->IsUsingNormalMap());
			NR_SERIALIZE_PROPERTY(Metalness, material->GetMetalness());
			NR_SERIALIZE_PROPERTY(Roughness, material->GetRoughness());

			{
				Ref<Texture2D> albedoMap = material->GetAlbedoMap();
				bool hasAlbedoMap = albedoMap ? !albedoMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
				AssetHandle albedoMapHandle = hasAlbedoMap ? albedoMap->Handle : (AssetHandle)0;
				NR_SERIALIZE_PROPERTY(AlbedoMap, albedoMapHandle);
			}
			{
				Ref<Texture2D> normalMap = material->GetNormalMap();
				bool hasNormalMap = normalMap ? !normalMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
				AssetHandle normalMapHandle = hasNormalMap ? normalMap->Handle : (AssetHandle)0;
				NR_SERIALIZE_PROPERTY(NormalMap, normalMapHandle);
			}
			{
				Ref<Texture2D> metalnessMap = material->GetMetalnessMap();
				bool hasMetalnessMap = metalnessMap ? !metalnessMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
				AssetHandle metalnessMapHandle = hasMetalnessMap ? metalnessMap->Handle : (AssetHandle)0;
				NR_SERIALIZE_PROPERTY(MetalnessMap, metalnessMapHandle);
			}
			{
				Ref<Texture2D> roughnessMap = material->GetRoughnessMap();
				bool hasRoughnessMap = roughnessMap ? !roughnessMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
				AssetHandle roughnessMapHandle = hasRoughnessMap ? roughnessMap->Handle : (AssetHandle)0;
				NR_SERIALIZE_PROPERTY(RoughnessMap, roughnessMapHandle);
			}
			out << YAML::EndMap;
		}
		out << YAML::EndMap; // Material

		std::ofstream fout(AssetManager::GetFileSystemPath(metadata));
		fout << out.c_str();
#undef NR_SERIALIZE_PROPERTY
	}

	bool MaterialAssetSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		std::ifstream stream(AssetManager::GetFileSystemPath(metadata));
		if (!stream.is_open())
		{
			return false;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();
		YAML::Node root = YAML::Load(strStream.str());
		YAML::Node materialNode = root["Material"];

		Ref<MaterialAsset> material = Ref<MaterialAsset>::Create();

#define NR_DESERIALIZE_PROPERTY(propName, destination, node, defaultValue) destination = node[#propName] ? node[#propName].as<decltype(defaultValue)>() : defaultValue

		NR_DESERIALIZE_PROPERTY(AlbedoColor, material->GetAlbedoColor(), materialNode, glm::vec3(0.8f));
		NR_DESERIALIZE_PROPERTY(Emission, material->GetEmission(), materialNode, 0.0f);
		material->SetUseNormalMap(materialNode["UseNormalMap"] ? materialNode["UseNormalMap"].as<bool>() : false);
		NR_DESERIALIZE_PROPERTY(Metalness, material->GetMetalness(), materialNode, 0.0f);
		NR_DESERIALIZE_PROPERTY(Roughness, material->GetRoughness(), materialNode, 0.5f);

		AssetHandle albedoMap, normalMap, metalnessMap, roughnessMap;
		NR_DESERIALIZE_PROPERTY(AlbedoMap, albedoMap, materialNode, (AssetHandle)0);
		NR_DESERIALIZE_PROPERTY(NormalMap, normalMap, materialNode, (AssetHandle)0);
		NR_DESERIALIZE_PROPERTY(MetalnessMap, metalnessMap, materialNode, (AssetHandle)0);
		NR_DESERIALIZE_PROPERTY(RoughnessMap, roughnessMap, materialNode, (AssetHandle)0);

		if (albedoMap)
		{
			if (AssetManager::IsAssetHandleValid(albedoMap))
			{
				material->SetAlbedoMap(AssetManager::GetAsset<Texture2D>(albedoMap));
			}
		}
		if (normalMap)
		{
			if (AssetManager::IsAssetHandleValid(normalMap))
			{
				material->SetNormalMap(AssetManager::GetAsset<Texture2D>(normalMap));
			}
		}
		if (metalnessMap)
		{
			if (AssetManager::IsAssetHandleValid(metalnessMap))
			{
				material->SetMetalnessMap(AssetManager::GetAsset<Texture2D>(metalnessMap));
			}
		}
		if (roughnessMap)
		{
			if (AssetManager::IsAssetHandleValid(roughnessMap))
			{
				material->SetRoughnessMap(AssetManager::GetAsset<Texture2D>(roughnessMap));
			}
		}
#undef NR_DESERIALIZE_PROPERTY

		asset = material;
		asset->Handle = metadata.Handle;
		return true;
	}

	bool EnvironmentSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		auto [radiance, irradiance] = Renderer::CreateEnvironmentMap(AssetManager::GetFileSystemPathString(metadata));

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

		std::ofstream fout(AssetManager::GetFileSystemPath(metadata));
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
	
	void AudioFileSourceSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
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

	void SoundConfigSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		//TODO: Remove macro
#define NR_SERIALIZE_PROPERTY(propName, propVal) out << YAML::Key << #propName << YAML::Value << propVal
		
		Ref<Audio::SoundConfig> soundConfig = asset.As<Audio::SoundConfig>();

		YAML::Emitter out;
		out << YAML::BeginMap; // SoundConfig

		if (soundConfig->FileAsset)
		{
			NR_SERIALIZE_PROPERTY(AssetID, soundConfig->FileAsset->Handle);
		}

		NR_SERIALIZE_PROPERTY(IsLooping, (bool)soundConfig->Looping);
		NR_SERIALIZE_PROPERTY(VolumeMultiplier, soundConfig->VolumeMultiplier);
		NR_SERIALIZE_PROPERTY(PitchMultiplier, soundConfig->PitchMultiplier);
		NR_SERIALIZE_PROPERTY(MasterReverbSend, soundConfig->MasterReverbSend);
		NR_SERIALIZE_PROPERTY(LPFilterValue, soundConfig->LPFilterValue);
		NR_SERIALIZE_PROPERTY(HPFilterValue, soundConfig->HPFilterValue);

		out << YAML::Key << "Spatialization";
		out << YAML::BeginMap; // Spatialization

		auto& spatialConfig = soundConfig->Spatialization;
		NR_SERIALIZE_PROPERTY(Enabled, soundConfig->SpatializationEnabled);
		NR_SERIALIZE_PROPERTY(AttenuationModel, (int)spatialConfig.AttenuationMod);
		NR_SERIALIZE_PROPERTY(MinGain, spatialConfig.MinGain);
		NR_SERIALIZE_PROPERTY(MaxGain, spatialConfig.MaxGain);
		NR_SERIALIZE_PROPERTY(MinDistance, spatialConfig.MinDistance);
		NR_SERIALIZE_PROPERTY(MaxDistance, spatialConfig.MaxDistance);
		NR_SERIALIZE_PROPERTY(ConeInnerAngle, spatialConfig.ConeInnerAngleInRadians);
		NR_SERIALIZE_PROPERTY(ConeOuterAngle, spatialConfig.ConeOuterAngleInRadians);
		NR_SERIALIZE_PROPERTY(ConeOuterGain, spatialConfig.ConeOuterGain);
		NR_SERIALIZE_PROPERTY(DopplerFactor, spatialConfig.DopplerFactor);
		NR_SERIALIZE_PROPERTY(Rollor, spatialConfig.Rolloff);
		NR_SERIALIZE_PROPERTY(AirAbsorptionEnabled, spatialConfig.AirAbsorptionEnabled);

		out << YAML::EndMap; // Spatialization
		out << YAML::EndMap; // SoundConfig

		std::ofstream fout(AssetManager::GetFileSystemPath(metadata));
		fout << out.c_str();

#undef NR_SERIALIZE_PROPERTY
	}

	bool SoundConfigSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		std::ifstream stream(AssetManager::GetFileSystemPath(metadata));
		if (!stream.is_open())
		{
			return false;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		auto soundConfig = Ref<Audio::SoundConfig>::Create();

		AssetHandle assetHandle = data["AssetID"] ? data["AssetID"].as<uint64_t>() : 0;
		if (AssetManager::IsAssetHandleValid(assetHandle))
		{
			soundConfig->FileAsset = AssetManager::GetAsset<AudioFile>(assetHandle);
		}
		else
		{
			NR_CORE_ERROR("Tried to load invalid Audio File asset in SoundConfig: {0}", metadata.FilePath.string());
		}

#define NR_DESERIALIZE_PROPERTY(propName, destination, data, defaultValue) destination = data[#propName] ? data[#propName].as<decltype(defaultValue)>() : defaultValue
		
		NR_DESERIALIZE_PROPERTY(IsLooping, soundConfig->Looping, data, false);
		NR_DESERIALIZE_PROPERTY(VolumeMultiplier, soundConfig->VolumeMultiplier, data, 1.0f);
		NR_DESERIALIZE_PROPERTY(PitchMultiplier, soundConfig->PitchMultiplier, data, 1.0f);
		NR_DESERIALIZE_PROPERTY(MasterReverbSend, soundConfig->MasterReverbSend, data, 0.0f);
		NR_DESERIALIZE_PROPERTY(LPFilterValue, soundConfig->LPFilterValue, data, 20000.0f);
		NR_DESERIALIZE_PROPERTY(HPFilterValue, soundConfig->HPFilterValue, data, 0.0f);

		auto spConfigData = data["Spatialization"];
		if (spConfigData)
		{
			soundConfig->SpatializationEnabled = spConfigData["Enabled"] ? spConfigData["Enabled"].as<bool>() : false;
			auto& spatialConfig = soundConfig->Spatialization;
			NR_DESERIALIZE_PROPERTY(Enabled, soundConfig->SpatializationEnabled, spConfigData, false);

			spatialConfig.AttenuationMod = spConfigData["AttenuationModel"] ? 
				static_cast<Audio::AttenuationModel>(spConfigData["AttenuationModel"].as<int>())
				: Audio::AttenuationModel::Inverse;

			NR_DESERIALIZE_PROPERTY(MinGain, spatialConfig.MinGain, spConfigData, 0.0f);
			NR_DESERIALIZE_PROPERTY(MaxGain, spatialConfig.MaxGain, spConfigData, 1.0f);
			NR_DESERIALIZE_PROPERTY(MinDistance, spatialConfig.MinDistance, spConfigData, 1.0f);
			NR_DESERIALIZE_PROPERTY(MaxDistance, spatialConfig.MaxDistance, spConfigData, 1000.0f);
			NR_DESERIALIZE_PROPERTY(ConeInnerAngle, spatialConfig.ConeInnerAngleInRadians, spConfigData, 6.283185f);
			NR_DESERIALIZE_PROPERTY(ConeOuterAngle, spatialConfig.ConeOuterAngleInRadians, spConfigData, 6.283185f);
			NR_DESERIALIZE_PROPERTY(ConeOuterGain, spatialConfig.ConeOuterGain, spConfigData, 0.0f);
			NR_DESERIALIZE_PROPERTY(DopplerFactor, spatialConfig.DopplerFactor, spConfigData, 1.0f);
			NR_DESERIALIZE_PROPERTY(Rollor, spatialConfig.Rolloff, spConfigData, 1.0f);
			NR_DESERIALIZE_PROPERTY(AirAbsorptionEnabled, spatialConfig.AirAbsorptionEnabled, spConfigData, true);
		}

#undef NR_DESERIALIZE_PROPERTY

		asset = soundConfig;
		asset->Handle = metadata.Handle;

		return true;
	}
}