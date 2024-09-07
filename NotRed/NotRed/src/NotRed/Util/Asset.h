#pragma once

#include "NotRed/Core/UUID.h"

namespace NR
{
	enum class AssetType
	{
		Scene,
		Mesh, Texture, EnvMap, 
		Audio, 
		Script, 
		Other
	};

	using AssetHandle = size_t;

	class Asset : public RefCounted
	{
	public:
		AssetHandle Handle;
		AssetType Type;

		std::string FilePath;
		std::string FileName;
		std::string Extension;
		int ParentDirectory;

		virtual ~Asset() = default;
	};

}