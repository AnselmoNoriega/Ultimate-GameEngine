#pragma once

#include <map>
#include <unordered_map>

#include "AssetSerializer.h"
#include "NotRed/Util/FileSystem.h"
#include "NotRed/Util/StringUtils.h"

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

		static SearchResults SearchFiles(const std::string& query, const std::string& searchPath);
		static std::string GetParentPath(const std::string& path);

		static bool IsDirectory(const std::string& filepath);

		static AssetHandle GetAssetIDForFile(const std::string& filepath);
		static bool IsAssetHandleValid(AssetHandle assetHandle);

		template<typename T, typename... Args>
		static Ref<T> CreateAsset(const std::string& filename, AssetType type, int directoryIndex, Args&&... args)
		{
			static_assert(std::is_base_of<Asset, T>::value, "CreateAsset only works for types derived from Asset");

			auto& directory = GetDirectoryInfo(directoryIndex);

			Ref<T> asset = Ref<T>::Create(std::forward<Args>(args)...);
			asset->Type = type;
			asset->FilePath = directory.FilePath + "/" + filename;
			asset->FileName = Utils::RemoveExtension(Utils::GetFilename(asset->FilePath));
			asset->Extension = Utils::GetFilename(filename);
			asset->ParentDirectory = directoryIndex;
			asset->Handle = std::hash<std::string>()(asset->FilePath);
			sLoadedAssets[asset->Handle] = asset;

			AssetSerializer::SerializeAsset(asset);

			return asset;
		}

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

		static std::string StripExtras(const std::string& filename);

	private:
		static void ImportAsset(const std::string& filepath, bool reimport = false, int parentIndex = -1);
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