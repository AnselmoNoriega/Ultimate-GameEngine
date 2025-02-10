#pragma once

#include <unordered_map>
#include <filesystem>

#include "AssetMetadata.h"

namespace NR
{
	class AssetRegistry
	{
	public:
		AssetMetadata& operator[](const std::filesystem::path& path);

		const AssetMetadata& Get(const std::filesystem::path& path) const;

		size_t Count() const { return mAssetRegistry.size(); }
		bool Contains(const std::filesystem::path& path) const;
		size_t Remove(const std::filesystem::path& path);

		void Clear();

		std::unordered_map<std::filesystem::path, AssetMetadata>::iterator begin() { return mAssetRegistry.begin(); }
		std::unordered_map<std::filesystem::path, AssetMetadata>::iterator end() { return mAssetRegistry.end(); }
		std::unordered_map<std::filesystem::path, AssetMetadata>::const_iterator cbegin() const { return mAssetRegistry.cbegin(); }
		std::unordered_map<std::filesystem::path, AssetMetadata>::const_iterator cend() const { return mAssetRegistry.cend(); }

	private:
		std::unordered_map<std::filesystem::path, AssetMetadata> mAssetRegistry;
	};
}