#include "nrpch.h"
#include "AssetImporter.h"

#include "AssetManager.h"
#include "MeshSerializer.h"

namespace NR
{
	void AssetImporter::Init()
	{
		sSerializers[AssetType::Texture] = CreateScope<TextureSerializer>();
		sSerializers[AssetType::MeshAsset] = CreateScope<MeshAssetSerializer>();
		sSerializers[AssetType::Mesh] = CreateScope<MeshSerializer>();
		sSerializers[AssetType::EnvMap] = CreateScope<EnvironmentSerializer>();
		sSerializers[AssetType::PhysicsMat] = CreateScope<PhysicsMaterialSerializer>();
		sSerializers[AssetType::Audio] = CreateScope<AudioFileSourceSerializer>();
	}

	void AssetImporter::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset)
	{
		if (sSerializers.find(metadata.Type) == sSerializers.end())
		{
			NR_CORE_WARN("There's currently no importer for assets of type {0}", metadata.FilePath.extension().string());
			return;
		}

		sSerializers[asset->GetAssetType()]->Serialize(metadata, asset);
	}

	void AssetImporter::Serialize(const Ref<Asset>& asset)
	{
		const AssetMetadata& metadata = AssetManager::GetMetadata(asset->Handle);
		Serialize(metadata, asset);
	}

	bool AssetImporter::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset)
	{
		if (sSerializers.find(metadata.Type) == sSerializers.end())
		{
			NR_CORE_WARN("There's currently no importer for assets of type {0}", metadata.FilePath.extension().string());
			return false;
		}

		return sSerializers[metadata.Type]->TryLoadData(metadata, asset);
	}

	std::unordered_map<AssetType, Scope<AssetSerializer>> AssetImporter::sSerializers;
}