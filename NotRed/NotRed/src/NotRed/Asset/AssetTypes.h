#pragma once

#include "NotRed/Core/Core.h"

namespace NR
{
	enum class AssetFlag : uint16_t
	{
		None = 0 << 0,
		Missing = 1 << 0,
		Invalid = 1 << 1
	};
	enum class AssetType
	{
		Scene,
		Mesh,
		Texture,
		EnvMap,
		MeshAsset,
		Audio,
		SoundConfig,
		SpatializationConfig,
		PhysicsMat,
		Other = -1
	};
}