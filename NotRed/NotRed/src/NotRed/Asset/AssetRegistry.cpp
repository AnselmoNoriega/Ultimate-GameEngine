#include "nrpch.h"
#include "AssetRegistry.h"

#include "NotRed/Project/Project.h"

namespace NR
{
#define NR_ASSETREGISTRY_LOG 0
#if NR_ASSETREGISTRY_LOG
	#define ASSET_LOG(...) NR_CORE_TRACE(__VA_ARGS__)
#else 
	#define ASSET_LOG(...)
#endif

	std::filesystem::path AssetRegistry::GetPathKey(const std::filesystem::path& path) const
	{
		auto key = std::filesystem::relative(path, Project::GetAssetDirectory());
		if (key.empty())
		{
			key = path.lexically_normal();
		}

		return key;
	}
	AssetMetadata& AssetRegistry::operator[](const std::filesystem::path& path)
	{
		auto key = GetPathKey(path);
		
		ASSET_LOG("[ASSET] Retrieving key {0} (path = {1})", key.string(), path.string());
		NR_CORE_ASSERT(!path.string().empty());
		
		return mAssetRegistry[key];
	}

	const AssetMetadata& AssetRegistry::Get(const std::filesystem::path& path) const
	{
		auto key = GetPathKey(path);
		
		NR_CORE_ASSERT(mAssetRegistry.find(key) != mAssetRegistry.end());
		ASSET_LOG("[ASSET] Retrieving const {0} (path = {1})", key.string(), path.string());
		NR_CORE_ASSERT(!path.string().empty());
		
		return mAssetRegistry.at(key);
	}

	bool AssetRegistry::Contains(const std::filesystem::path& path) const
	{
		auto key = GetPathKey(path);
		
		ASSET_LOG("[ASSET] Contains key {0} (path = {1})", key.string(), path.string());
		
		return mAssetRegistry.find(key) != mAssetRegistry.end();
	}

	size_t AssetRegistry::Remove(const std::filesystem::path& path)
	{
		auto key = GetPathKey(path);
		
		ASSET_LOG("[ASSET] Removing key {0} (path = {1})", key.string(), path.string());
		
		return mAssetRegistry.erase(key);
	}

	void AssetRegistry::Clear()
	{
		ASSET_LOG("[ASSET] Clearing registry");
		
		mAssetRegistry.clear();
	}
}