#pragma once

#include <unordered_map>

#include "AssetTypes.h"

namespace NR
{
	inline static std::unordered_map<std::string, AssetType> sAssetExtensionMap =
	{
		{ ".nrsc", AssetType::Scene },
		{ ".nrmaterial", AssetType::Material },
		{ ".nrmesh", AssetType::Mesh },
		{ ".nrsound", AssetType::SoundConfig },
		{ ".nrpm", AssetType::PhysicsMat },
		{ ".fbx", AssetType::MeshAsset },
		{ ".gltf", AssetType::MeshAsset },
		{ ".glb", AssetType::MeshAsset },
		{ ".obj", AssetType::MeshAsset },
		{ ".png", AssetType::Texture },
		{ ".jpg", AssetType::Texture },
		{ ".jpeg", AssetType::Texture },
		{ ".hdr", AssetType::EnvMap },
		{ ".wav", AssetType::Audio },
		{ ".ogg", AssetType::Audio }
	};
}