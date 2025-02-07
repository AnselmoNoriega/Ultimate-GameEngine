#include "nrpch.h"
#include "AssetSerializer.h"

#include "NotRed/Util/StringUtils.h"
#include "NotRed/Util/FileSystem.h"

#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Renderer/Renderer.h"
#include "NotRed/Renderer/MaterialAsset.h"
#include "NotRed/Scene/SceneSerializer.h"

#include "NotRed/Scene/Prefab.h"

#include "NotRed/Renderer/Animation.h"
#include "NotRed/Audio/AudioFileUtils.h"
#include "NotRed/Audio/Sound.h"
#include "NotRed/Util/SerializationMacros.h"

#include "AssetManager.h"

#include "yaml-cpp/yaml.h"
#include "NotRed/Util/YAMLSerializationHelpers.h"
#include "NotRed/Util/SerializationMacros.h"

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

	bool FontSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		asset = Ref<Font>::Create(AssetManager::GetFileSystemPathString(metadata));
		asset->Handle = metadata.Handle;

		return true;
	}

	bool MeshAssetSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		asset = Ref<MeshAsset>::Create(AssetManager::GetFileSystemPathString(metadata));
		asset->Handle = metadata.Handle;
		return (asset.As<MeshAsset>())->GetStaticVertices().size() > 0;
	}

	void MaterialAssetSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<MaterialAsset> material = asset.As<MaterialAsset>();
		
		YAML::Emitter out;
		out << YAML::BeginMap; // Material
		out << YAML::Key << "Material" << YAML::Value;
		{
			out << YAML::BeginMap;

			NR_SERIALIZE_PROPERTY(AlbedoColor, material->GetAlbedoColor(), out);
			NR_SERIALIZE_PROPERTY(Emission, material->GetEmission(), out);
			NR_SERIALIZE_PROPERTY(UseNormalMap, material->IsUsingNormalMap(), out);
			NR_SERIALIZE_PROPERTY(Metalness, material->GetMetalness(), out);
			NR_SERIALIZE_PROPERTY(Roughness, material->GetRoughness(), out);

			{
				Ref<Texture2D> albedoMap = material->GetAlbedoMap();
				bool hasAlbedoMap = albedoMap ? !albedoMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
				AssetHandle albedoMapHandle = hasAlbedoMap ? albedoMap->Handle : (AssetHandle)0;
				NR_SERIALIZE_PROPERTY(AlbedoMap, albedoMapHandle, out);
			}
			{
				Ref<Texture2D> normalMap = material->GetNormalMap();
				bool hasNormalMap = normalMap ? !normalMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
				AssetHandle normalMapHandle = hasNormalMap ? normalMap->Handle : (AssetHandle)0;
				NR_SERIALIZE_PROPERTY(NormalMap, normalMapHandle, out);
			}
			{
				Ref<Texture2D> metalnessMap = material->GetMetalnessMap();
				bool hasMetalnessMap = metalnessMap ? !metalnessMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
				AssetHandle metalnessMapHandle = hasMetalnessMap ? metalnessMap->Handle : (AssetHandle)0;
				NR_SERIALIZE_PROPERTY(MetalnessMap, metalnessMapHandle, out);
			}
			{
				Ref<Texture2D> roughnessMap = material->GetRoughnessMap();
				bool hasRoughnessMap = roughnessMap ? !roughnessMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
				AssetHandle roughnessMapHandle = hasRoughnessMap ? roughnessMap->Handle : (AssetHandle)0;
				NR_SERIALIZE_PROPERTY(RoughnessMap, roughnessMapHandle, out);
			}
			out << YAML::EndMap;
		}
		out << YAML::EndMap; // Material

		std::ofstream fout(AssetManager::GetFileSystemPath(metadata));
		fout << out.c_str();
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
		Ref<SoundConfig> soundConfig = asset.As<SoundConfig>();

		YAML::Emitter out;
		out << YAML::BeginMap; // SoundConfig

		if (soundConfig->FileAsset)
		{
			NR_SERIALIZE_PROPERTY(AssetID, soundConfig->FileAsset->Handle, out);
		}

		NR_SERIALIZE_PROPERTY(IsLooping, (bool)soundConfig->bLooping, out);
		NR_SERIALIZE_PROPERTY(VolumeMultiplier, soundConfig->VolumeMultiplier, out);
		NR_SERIALIZE_PROPERTY(PitchMultiplier, soundConfig->PitchMultiplier, out);
		NR_SERIALIZE_PROPERTY(MasterReverbSend, soundConfig->MasterReverbSend, out);
		NR_SERIALIZE_PROPERTY(LPFilterValue, soundConfig->LPFilterValue, out);
		NR_SERIALIZE_PROPERTY(HPFilterValue, soundConfig->HPFilterValue, out);

		out << YAML::Key << "Spatialization";
		out << YAML::BeginMap; // Spatialization

		auto& spatialConfig = soundConfig->Spatialization;
		NR_SERIALIZE_PROPERTY(Enabled, soundConfig->bSpatializationEnabled, out);
		NR_SERIALIZE_PROPERTY(AttenuationModel, (int)spatialConfig->AttenuationMod, out);
		NR_SERIALIZE_PROPERTY(MinGain, spatialConfig->MinGain, out);
		NR_SERIALIZE_PROPERTY(MaxGain, spatialConfig->MaxGain, out);
		NR_SERIALIZE_PROPERTY(MinDistance, spatialConfig->MinDistance, out);
		NR_SERIALIZE_PROPERTY(MaxDistance, spatialConfig->MaxDistance, out);
		NR_SERIALIZE_PROPERTY(ConeInnerAngle, spatialConfig->ConeInnerAngleInRadians, out);
		NR_SERIALIZE_PROPERTY(ConeOuterAngle, spatialConfig->ConeOuterAngleInRadians, out);
		NR_SERIALIZE_PROPERTY(ConeOuterGain, spatialConfig->ConeOuterGain, out);
		NR_SERIALIZE_PROPERTY(DopplerFactor, spatialConfig->DopplerFactor, out);
		NR_SERIALIZE_PROPERTY(Rollor, spatialConfig->Rolloff, out);
		NR_SERIALIZE_PROPERTY(AirAbsorptionEnabled, spatialConfig->bAirAbsorptionEnabled, out);

		out << YAML::EndMap; // Spatialization
		out << YAML::EndMap; // SoundConfig

		std::ofstream fout(AssetManager::GetFileSystemPath(metadata));
		fout << out.c_str();
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
		auto soundConfig = Ref<SoundConfig>::Create();

		AssetHandle assetHandle = data["AssetID"] ? data["AssetID"].as<uint64_t>() : 0;
		if (AssetManager::IsAssetHandleValid(assetHandle))
		{
			soundConfig->FileAsset = AssetManager::GetAsset<AudioFile>(assetHandle);
		}
		else if (assetHandle != 0)
		{
			NR_CORE_ERROR("Tried to load invalid Audio File asset in SoundConfig: {0}", metadata.FilePath.string());
		}

		NR_DESERIALIZE_PROPERTY(IsLooping, soundConfig->bLooping, data, false);
		NR_DESERIALIZE_PROPERTY(VolumeMultiplier, soundConfig->VolumeMultiplier, data, 1.0f);
		NR_DESERIALIZE_PROPERTY(PitchMultiplier, soundConfig->PitchMultiplier, data, 1.0f);
		NR_DESERIALIZE_PROPERTY(MasterReverbSend, soundConfig->MasterReverbSend, data, 0.0f);
		NR_DESERIALIZE_PROPERTY(LPFilterValue, soundConfig->LPFilterValue, data, 20000.0f);
		NR_DESERIALIZE_PROPERTY(HPFilterValue, soundConfig->HPFilterValue, data, 0.0f);

		auto spConfigData = data["Spatialization"];
		if (spConfigData)
		{
			soundConfig->bSpatializationEnabled = spConfigData["Enabled"] ? spConfigData["Enabled"].as<bool>() : false;
			auto& spatialConfig = soundConfig->Spatialization;
			NR_DESERIALIZE_PROPERTY(Enabled, soundConfig->bSpatializationEnabled, spConfigData, false);

			spatialConfig->AttenuationMod = spConfigData["AttenuationModel"] ? static_cast<AttenuationModel>(spConfigData["AttenuationModel"].as<int>())
				: AttenuationModel::Inverse;
			NR_DESERIALIZE_PROPERTY(MinGain, spatialConfig->MinGain, spConfigData, 0.0f);
			NR_DESERIALIZE_PROPERTY(MaxGain, spatialConfig->MaxGain, spConfigData, 1.0f);
			NR_DESERIALIZE_PROPERTY(MinDistance, spatialConfig->MinDistance, spConfigData, 1.0f);
			NR_DESERIALIZE_PROPERTY(MaxDistance, spatialConfig->MaxDistance, spConfigData, 1000.0f);
			NR_DESERIALIZE_PROPERTY(ConeInnerAngle, spatialConfig->ConeInnerAngleInRadians, spConfigData, 6.283185f);
			NR_DESERIALIZE_PROPERTY(ConeOuterAngle, spatialConfig->ConeOuterAngleInRadians, spConfigData, 6.283185f);
			NR_DESERIALIZE_PROPERTY(ConeOuterGain, spatialConfig->ConeOuterGain, spConfigData, 0.0f);
			NR_DESERIALIZE_PROPERTY(DopplerFactor, spatialConfig->DopplerFactor, spConfigData, 1.0f);
			NR_DESERIALIZE_PROPERTY(Rollor, spatialConfig->Rolloff, spConfigData, 1.0f);
			NR_DESERIALIZE_PROPERTY(AirAbsorptionEnabled, spatialConfig->bAirAbsorptionEnabled, spConfigData, true);
			NR_DESERIALIZE_PROPERTY(SpreadFromSourceSize, spatialConfig->bSpreadFromSourceSize, spConfigData, true);
			NR_DESERIALIZE_PROPERTY(SourceSize, spatialConfig->SourceSize, spConfigData, 1.0f);
			NR_DESERIALIZE_PROPERTY(Spread, spatialConfig->Spread, spConfigData, 1.0f);
			NR_DESERIALIZE_PROPERTY(Focus, spatialConfig->Focus, spConfigData, 1.0f);
		}

#undef NR_DESERIALIZE_PROPERTY

		asset = soundConfig;
		asset->Handle = metadata.Handle;

		return true;
	}

	void PrefabSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<Prefab> prefab = asset.As<Prefab>();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Prefab";
		out << YAML::Value << YAML::BeginSeq;
		prefab->mScene->mRegistry.view<IDComponent>().each([&](auto entityID, auto& idComponent)
			{
				Entity entity = { entityID, prefab->mScene.Raw() };
				if (!entity)
				{
					return;
				}
				SceneSerializer::SerializeEntity(out, entity);
			});

		out << YAML::EndSeq;
		out << YAML::EndMap;
		std::ofstream fout(AssetManager::GetFileSystemPath(metadata));
		fout << out.c_str();
	}

	bool PrefabSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		std::ifstream stream(AssetManager::GetFileSystemPath(metadata));
		if (!stream.is_open())
		{
			return false;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();
		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Prefab"])
		{
			return false;
		}

		YAML::Node prefabNode = data["Prefab"];
		Ref<Prefab> prefab = Ref<Prefab>::Create();
		SceneSerializer::DeserializeEntities(prefabNode, prefab->mScene);
		asset = prefab;
		asset->Handle = metadata.Handle;

		return true;
	}

	void SceneAssetSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		SceneSerializer serializer(asset.As<Scene>());
		serializer.Serialize(AssetManager::GetFileSystemPath(metadata).string());
	}

	bool SceneAssetSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		asset = Ref<Asset>::Create();
		asset->Handle = metadata.Handle;
		return true;
	}

	bool SkeletonAssetSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		Ref<Asset> temp = asset;
		asset = Ref<SkeletonAsset>::Create(AssetManager::GetFileSystemPathString(metadata));
		asset->Handle = metadata.Handle;
		return (asset.As<SkeletonAsset>())->IsValid();
	}

	bool AnimationAssetSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		Ref<Asset> temp = asset;
		asset = Ref<AnimationAsset>::Create(AssetManager::GetFileSystemPathString(metadata));
		asset->Handle = metadata.Handle;
		return (asset.As<AnimationAsset>())->IsValid();
	}

	void AnimationControllerSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<AnimationController> animationController = asset.As<AnimationController>();
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "AnimationController" << YAML::Value;
		{
			out << YAML::BeginMap;
			out << YAML::Key << "AssetHandle" << YAML::Value << animationController->Handle;
			out << YAML::Key << "IsPlaying" << YAML::Value << animationController->IsAnimationPlaying();
			out << YAML::Key << "SkeletonAsset" << YAML::Value << animationController->GetSkeletonAsset()->Handle;
			out << YAML::Key << "States" << YAML::Value;
			out << YAML::BeginSeq;
			{
				for (size_t i = 0; i < animationController->GetNumStates(); ++i)
				{
					out << YAML::BeginMap;
					{
						out << YAML::Key << "State" << YAML::Value << animationController->GetStateName(i);
						out << YAML::Key << "AnimationAsset" << YAML::Value << animationController->GetAnimationAsset(i)->Handle;
					}
					out << YAML::EndMap;
				}
			}
			out << YAML::EndSeq;
		}
		out << YAML::EndMap;
		std::ofstream fout(AssetManager::GetFileSystemPath(metadata));
		fout << out.c_str();
	}

	bool AnimationControllerSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		std::ifstream stream(AssetManager::GetFileSystemPath(metadata));
		if (!stream.is_open())
		{
			return false;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();
		YAML::Node data = YAML::Load(strStream.str());
		if (!data["AnimationController"])
		{
			return false;
		}

		YAML::Node animationControllerNode = data["AnimationController"];
		Ref<AnimationAsset> animationAsset;

		Ref<AnimationController> animationController = Ref<AnimationController>::Create();
		if (animationControllerNode["IsPlaying"])
		{
			animationController->SetAnimationPlaying(animationControllerNode["IsPlaying"].as<bool>());
		}
		if (animationControllerNode["SkeletonAsset"])
		{
			animationController->SetSkeletonAsset(AssetManager::GetAsset<SkeletonAsset>(animationControllerNode["SkeletonAsset"].as<uint64_t>()));
		}
		else
		{
			return false;
		}
		for (const auto& state : animationControllerNode["States"]) 
		{
			if (!state.IsMap() || !state["AnimationAsset"] || !state["Name"])
			{
				continue;
			}

			animationController->SetAnimationAsset(state["Name"].as<std::string>(), AssetManager::GetAsset<AnimationAsset>(state["AnimationAsset"].as<uint64_t>()));
		}

		animationController->Handle = metadata.Handle;
		asset = animationController;
		return true;
	}
}