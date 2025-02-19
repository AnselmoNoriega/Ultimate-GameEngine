#pragma once

#include <unordered_map>

#include "AssetTypes.h"

namespace NR
{
	inline static std::unordered_map<std::string, AssetType> sAssetExtensionMap =
	{
		// NotRed types
		{ ".nrsc", AssetType::Scene },
		{ ".nrmaterial", AssetType::Material },
		{ ".nrprefab", AssetType::Prefab },
		{ ".nrmesh", AssetType::Mesh },
		{ ".nrsmesh", AssetType::StaticMesh },
		{ ".nrsound", AssetType::SoundConfig },
		{ ".nrpm", AssetType::PhysicsMat },
		{ ".nranim", AssetType::AnimationController },
		
		// Meshes
		{ ".fbx", AssetType::MeshSource },
		{ ".gltf", AssetType::MeshSource },
		{ ".glb", AssetType::MeshSource },
		{ ".obj", AssetType::MeshSource },

		// Textures
		{ ".png", AssetType::Texture },
		{ ".jpg", AssetType::Texture },
		{ ".jpeg", AssetType::Texture },
		{ ".hdr", AssetType::EnvMap },

		// Audio
		{ ".wav", AssetType::Audio },
		{ ".ogg", AssetType::Audio },

		// Fonts
		{ ".ttf", AssetType::Font },
		{ ".ttc", AssetType::Font },
		{ ".otf", AssetType::Font },

		// Graphs
		{ ".soul_sound", AssetType::SOULSound }
	};
}