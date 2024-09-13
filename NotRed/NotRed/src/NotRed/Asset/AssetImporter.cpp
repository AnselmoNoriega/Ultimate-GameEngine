#include "nrpch.h"
#include "AssetImporter.h"

namespace NR
{
	void AssetImporter::Init()
	{
		sSerializers[AssetType::Texture] = CreateScope<TextureSerializer>();
		sSerializers[AssetType::Mesh] = CreateScope<MeshSerializer>();
		sSerializers[AssetType::EnvMap] = CreateScope<EnvironmentSerializer>();
		sSerializers[AssetType::PhysicsMat] = CreateScope<PhysicsMaterialSerializer>();
	}

	void AssetImporter::Serialize(const Ref<Asset>& asset)
	{
		if (sSerializers.find(asset->Type) == sSerializers.end())
		{
			NR_CORE_WARN("There's currently no importer for assets of type {0}", asset->Extension);
			return;
		}

		sSerializers[asset->Type]->Serialize(asset);
	}

	bool AssetImporter::TryLoadData(Ref<Asset>& asset)
	{
		if (asset->Type == AssetType::Directory)
			return false;

		if (sSerializers.find(asset->Type) == sSerializers.end())
		{
			NR_CORE_WARN("There's currently no importer for assets of type {0}", asset->Extension);
			return false;
		}

		return sSerializers[asset->Type]->TryLoadData(asset);
	}

	std::unordered_map<AssetType, Scope<AssetSerializer>> AssetImporter::sSerializers;

}