#pragma once

#include <filesystem>

#include "Asset.h"

namespace NR
{
	struct AssetMetadata
	{
		AssetHandle Handle = 0;
		AssetType Type;

		std::filesystem::path FilePath;
		std::string FileName;
		std::string Extension;

		bool IsDataLoaded = false;
		bool IsValid() const { return Handle != 0; }
	};
}