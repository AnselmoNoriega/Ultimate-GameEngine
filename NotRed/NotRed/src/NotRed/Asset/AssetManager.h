#pragma once

#include <map>
#include <unordered_map>

#include "AssetImporter.h"
#include "NotRed/Util/FileSystem.h"
#include "NotRed/Util/StringUtils.h"
#include "NotRed/Project/Project.h"

namespace NR
{
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
        using AssetsChangeEventFn = std::function<void(FileSystemChangedEvent)>;

    public:
        static void Init();
        static void Shutdown();

        static void SetAssetChangeCallback(const AssetsChangeEventFn& callback);

        static AssetMetadata& GetMetadata(AssetHandle handle);
        static AssetMetadata& GetMetadata(const std::string& filepath);

        static AssetHandle GetAssetHandleFromFilePath(const std::string& filepath);
        static bool IsAssetHandleValid(AssetHandle assetHandle) { return GetMetadata(assetHandle).IsValid(); }

        static AssetType GetAssetTypeForFileType(const std::string& extension);

        static AssetHandle ImportAsset(const std::string& filepath);

        static std::string GetFileSystemPath(const AssetMetadata& metadata) { return (Project::GetAssetDirectory() / metadata.FilePath).string(); }

        template<typename T, typename... Args>
        static Ref<T> CreateNewAsset(const std::string& filename, const std::string& directoryPath, Args&&... args)
        {
            static_assert(std::is_base_of<Asset, T>::value, "CreateNewAsset only works for types derived from Asset");

            FileSystem::SkipNextFileSystemChange();

            AssetMetadata metadata;
            metadata.Handle = AssetHandle();
            metadata.FilePath = directoryPath + "/" + filename;
            metadata.FileName = Utils::RemoveExtension(Utils::GetFilename(metadata.FilePath));
            metadata.Extension = Utils::GetExtension(filename);
            metadata.IsDataLoaded = true;
            metadata.Type = T::GetStaticType();

            if (FileSystem::Exists(metadata.FilePath))
            {
                bool foundAvailableFileName = false;
                int current = 1;
                while (!foundAvailableFileName)
                {
                    std::string nextFilePath = directoryPath + "/" + metadata.FileName;
                    if (current < 10)
                    {
                        nextFilePath += " (0" + std::to_string(current) + ")";
                    }
                    else
                    {
                        nextFilePath += " (" + std::to_string(current) + ")";
                    }
                    nextFilePath += "." + metadata.Extension;
                    if (!FileSystem::Exists(nextFilePath))
                    {
                        foundAvailableFileName = true;
                        metadata.FilePath = nextFilePath;
                        metadata.FileName = Utils::RemoveExtension(Utils::GetFilename(metadata.FilePath));
                        break;
                    }
                    current++;
                }
            }

            sAssetRegistry[metadata.FilePath] = metadata;
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
            auto& metadata = GetMetadata(assetHandle);
            NR_CORE_ASSERT(metadata.IsValid());

            Ref<Asset> asset = nullptr;
            if (!metadata.IsDataLoaded)
            {
                metadata.IsDataLoaded = AssetImporter::TryLoadData(metadata, asset);
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

    private:
        static void LoadAssetRegistry();
        static void ProcessDirectory(const std::string& directoryPath);
        static void ReloadAssets();
        static void WriteRegistryToFile();

        static void FileSystemChanged(FileSystemChangedEvent e);

        static void AssetRenamed(AssetHandle assetHandle, const std::string& newFilePath);
        static void AssetMoved(AssetHandle assetHandle, const std::string& destinationPath);
        static void AssetDeleted(AssetHandle assetHandle);

    private:
        static std::unordered_map<AssetHandle, Ref<Asset>> sLoadedAssets;
        static std::unordered_map<std::string, AssetMetadata> sAssetRegistry;
        static AssetsChangeEventFn sAssetsChangeCallback;

    private:
        friend class ContentBrowserPanel;
        friend class ContentBrowserAsset;
    };
}