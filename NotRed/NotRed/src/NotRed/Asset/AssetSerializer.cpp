#include "nrpch.h"
#include "AssetSerializer.h"

#include "NotRed/Util/StringUtils.h"
#include "NotRed/Util/FileSystem.h"

#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Renderer/Renderer.h"

#include "NotRed/Audio/AudioFileUtils.h"
#include "NotRed/Audio/Sound.h"

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
			NR_CORE_ERROR("Tried to load invalid Audio File asset in SoundConfig: {0}", metadata.FilePath);
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