#pragma once

#include <map>
#include <unordered_map>

#include "NotRed/Project/Project.h"
#include "NotRed/Util/FileSystem.h"
#include "NotRed/Util/StringUtils.h"

#include "AssetImporter.h"
#include "AssetRegistry.h"

#include "NotRed/Debug/Profiler.h"

namespace NR
{
	// Deserialized from project file - these are just defaults
	class AssetManagerConfig
	{
		std::string AssetDirectory = "Assets/";
		std::string AssetRegistryPath = "Assets/AssetRegistry.nrr";

		std::string MeshPath = "Assets/Meshes/";
		std::string MeshSourcePath = "Assets/Meshes/Source/";
	};

	class AssetManager
	{
	public:
		using AssetsChangeEventFn = std::function<void(const std::vector<FileSystemChangedEvent>&)>;
	public:
		static void Init();
		static void SetAssetChangeCallback(const AssetsChangeEventFn& callback);
		static void Shutdown();

		static const AssetMetadata& GetMetadata(AssetHandle handle);
		static const AssetMetadata& GetMetadata(const std::filesystem::path& filepath);
		static const AssetMetadata& GetMetadata(const Ref<Asset>& asset) { return GetMetadata(asset->Handle); }

		static std::filesystem::path GetFileSystemPath(const AssetMetadata& metadata) { return Project::GetAssetDirectory() / metadata.FilePath; }
		static std::string GetFileSystemPathString(const AssetMetadata& metadata) { return GetFileSystemPath(metadata).string(); }
		static std::filesystem::path GetRelativePath(const std::filesystem::path& filepath);

		static AssetHandle GetAssetHandleFromFilePath(const std::filesystem::path& filepath);
		static bool IsAssetHandleValid(AssetHandle assetHandle) { return IsMemoryAsset(assetHandle) || GetMetadata(assetHandle).IsValid(); }

		static AssetType GetAssetTypeFromExtension(const std::string& extension);
		static AssetType GetAssetTypeFromPath(const std::filesystem::path& path);

		static AssetHandle ImportAsset(const std::filesystem::path& filepath);
		static bool ReloadData(AssetHandle assetHandle);

		template<typename T, typename... Args>
		static Ref<T> CreateNewAsset(const std::string& filename, const std::string& directoryPath, Args&&... args)
		{
			static_assert(std::is_base_of<Asset, T>::value, "CreateNewAsset only works for types derived from Asset");

			AssetMetadata metadata;
			metadata.Handle = AssetHandle();
			if (directoryPath.empty() || directoryPath == ".")
				metadata.FilePath = filename;
			else
				metadata.FilePath = AssetManager::GetRelativePath(directoryPath + "/" + filename);
			metadata.IsDataLoaded = true;
			metadata.Type = T::GetStaticType();

			if (FileExists(metadata))
			{
				bool foundAvailableFileName = false;
				int current = 1;

				while (!foundAvailableFileName)
				{
					std::string nextFilePath = directoryPath + "/" + metadata.FilePath.stem().string();
					if (current < 10)
						nextFilePath += " (0" + std::to_string(current) + ")";
					else
						nextFilePath += " (" + std::to_string(current) + ")";
					nextFilePath += metadata.FilePath.extension().string();

					if (!FileSystem::Exists(nextFilePath))
					{
						foundAvailableFileName = true;
						metadata.FilePath = nextFilePath;
						break;
					}

					current++;
				}
			}

			sAssetRegistry[metadata.FilePath.string()] = metadata;

			WriteRegistryToFile();

			Ref<T> asset = Ref<T>::Create(std::forward<Args>(args)...);
			asset->Handle = metadata.Handle;
			sLoadedAssets[asset->Handle] = asset;
			AssetImporter::Serialize(metadata, asset);

			return asset;
		}

		template<typename T>
		static Ref<T> GetAsset(AssetHandle assetHandle)
		{
			NR_PROFILE_FUNC();

			if (IsMemoryAsset(assetHandle))
				return sMemoryAssets[assetHandle].As<T>();

			auto& metadata = GetMetadataInternal(assetHandle);
			if (!metadata.IsValid())
				return nullptr;

			Ref<Asset> asset = nullptr;
			if (!metadata.IsDataLoaded)
			{
				metadata.IsDataLoaded = AssetImporter::TryLoadData(metadata, asset);
				if (!metadata.IsDataLoaded)
					return nullptr;

				sLoadedAssets[assetHandle] = asset;
			}
			else
			{
				asset = sLoadedAssets[assetHandle];
			}

			return asset.As<T>();
		}

		template<typename T>
		static Ref<T> GetAsset(const std::string& filepath)
		{
			return GetAsset<T>(GetAssetHandleFromFilePath(filepath));
		}

		static bool FileExists(AssetMetadata& metadata)
		{
			return FileSystem::Exists(Project::GetActive()->GetAssetDirectory() / metadata.FilePath);
		}

		static const std::unordered_map<AssetHandle, Ref<Asset>>& GetLoadedAssets() { return sLoadedAssets; }
		static const AssetRegistry& GetAssetRegistry() { return sAssetRegistry; }

		template<typename TAsset, typename... TArgs>
		static AssetHandle CreateMemoryOnlyAsset(TArgs&&... args)
		{
			static_assert(std::is_base_of<Asset, TAsset>::value, "CreateMemoryOnlyAsset only works for types derived from Asset");

			Ref<TAsset> asset = Ref<TAsset>::Create(std::forward<TArgs>(args)...);
			asset->Handle = AssetHandle();

			sMemoryAssets[asset->Handle] = asset;
			return asset->Handle;
		}

		static bool IsMemoryAsset(AssetHandle handle)
		{
			return sMemoryAssets.find(handle) != sMemoryAssets.end();
		}

		static void ImGuiRender(bool& open);

	private:
		static void LoadAssetRegistry();
		static void ProcessDirectory(const std::filesystem::path& directoryPath);
		static void ReloadAssets();
		static void WriteRegistryToFile();

		static AssetMetadata& GetMetadataInternal(AssetHandle handle);

		static void FileSystemChanged(const std::vector<FileSystemChangedEvent>& events);
		static void AssetRenamed(AssetHandle assetHandle, const std::filesystem::path& newFilePath);
		static void AssetMoved(AssetHandle assetHandle, const std::filesystem::path& destinationPath);
		static void AssetDeleted(AssetHandle assetHandle);

	private:
		static std::unordered_map<AssetHandle, Ref<Asset>> sLoadedAssets;
		static std::unordered_map<AssetHandle, Ref<Asset>> sMemoryAssets;
		static AssetsChangeEventFn sAssetsChangeCallback;
		inline static AssetRegistry sAssetRegistry;
	private:
		friend class ContentBrowserPanel;
		friend class ContentBrowserAsset;
		friend class ContentBrowserDirectory;
	};
}