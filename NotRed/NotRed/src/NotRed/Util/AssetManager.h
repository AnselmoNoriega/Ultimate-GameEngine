#pragma once

#include <map>
#include <unordered_map>

#include "NotRed/Util/FileSystem.h"
#include "NotRed/Util/Asset.h"

namespace NR
{
	class AssetTypes
	{
	public:
		static void Init();
		static size_t GetAssetTypeID(const std::string& extension);
		static AssetType GetAssetTypeFromExtension(const std::string& extension);

	private:
		static std::map<std::string, AssetType> sTypes;
	};

	struct DirectoryInfo
	{
		std::string DirectoryName;
		std::string FilePath;
		int DirectoryIndex;
		int ParentIndex;
		std::vector<int> ChildrenIndices;
	};

	struct SearchResults
	{
		std::vector<DirectoryInfo> Directories;
		std::vector<Ref<Asset>> Assets;
	};

	class AssetManager
	{
	public:
		using AssetsChangeEventFn = std::function<void()>;

	public:
		static void Init();
		static void Shutdown();

		static void SetAssetChangeCallback(const AssetsChangeEventFn& callback);
		static DirectoryInfo& GetDirectoryInfo(int index);
		static std::vector<Ref<Asset>> GetAssetsInDirectory(int dirIndex);
		static std::vector<std::string> GetDirectoryNames(const std::string& filepath);

		static SearchResults SearchFiles(const std::string& query, const std::string& searchPath);
		static std::string GetParentPath(const std::string& path);

		static bool IsDirectory(const std::string& filepath);

		static AssetHandle GetAssetIDForFile(const std::string& filepath);
		static bool IsAssetHandleValid(AssetHandle assetHandle);

		template<typename T>
		static Ref<T> GetAsset(AssetHandle assetHandle)
		{
			NR_CORE_ASSERT(sLoadedAssets.find(assetHandle) != sLoadedAssets.end());
			return (Ref<T>)sLoadedAssets[assetHandle];
		}

		static bool IsAssetType(AssetHandle assetHandle, AssetType type)
		{
			return sLoadedAssets.find(assetHandle) != sLoadedAssets.end() && sLoadedAssets[assetHandle]->Type == type;
		}

		static bool MoveFile(const std::string& originalPath, const std::string& dest);

		static std::string StripExtras(const std::string& filename);

	private:
		static std::string ParseFilename(const std::string& filepath, const std::string& delim);
		static std::string ParseFileType(const std::string& filename);
		static std::string RemoveExtension(const std::string& filename);

		static void ImportAsset(const std::string& filepath, bool reimport = false, int parentIndex = -1);
		static void CreateMetaFile(const Ref<Asset>& asset);
		static void LoadMetaData(Ref<Asset>& asset, const std::string& filepath);
		static int ProcessDirectory(const std::string& directoryPath, int parentIndex = -1);
		static void ReloadAssets();

		static void FileSystemChanged(FileSystemChangedEvent e);

		static int FindParentIndexInChildren(DirectoryInfo& dir, const std::string& dirName);
		static int FindParentIndex(const std::string& filepath);

	private:
		static std::unordered_map<AssetHandle, Ref<Asset>> sLoadedAssets;
		static std::vector<DirectoryInfo> sDirectories;
		static AssetsChangeEventFn sAssetsChangeCallback;
	};
}