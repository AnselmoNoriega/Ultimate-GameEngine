#pragma once

#include <map>
#include <unordered_map>

#include "AssetImporter.h"
#include "NotRed/Util/FileSystem.h"
#include "NotRed/Util/StringUtils.h"

namespace NR
{
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
        static bool IsAssetHandleValid(AssetHandle assetHandle) { return assetHandle != 0 && sLoadedAssets.find(assetHandle) != sLoadedAssets.end(); }

        static void Rename(AssetHandle assetHandle, const std::string& newName);
        static void RemoveAsset(AssetHandle assetHandle);

        static AssetType GetAssetTypeForFileType(const std::string& extension);

        static AssetHandle ImportAsset(const std::string& filepath);

        template<typename T, typename... Args>
        static Ref<T> CreateNewAsset(const std::string& filename, const std::string& directoryPath, Args&&... args)
        {
            static_assert(std::is_base_of<Asset, T>::value, "CreateNewAsset only works for types derived from Asset");

            AssetMetadata metadata;
            metadata.Handle = AssetHandle();
            metadata.FilePath = directoryPath + "/" + filename;
            metadata.FileName = Utils::RemoveExtension(Utils::GetFilename(metadata.FilePath));
            metadata.Extension = Utils::GetExtension(filename);
            metadata.IsDataLoaded = true;
            metadata.Type = T::GetStaticType();
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

    private:
        static std::unordered_map<AssetHandle, Ref<Asset>> sLoadedAssets;
        static std::unordered_map<std::string, AssetMetadata> sAssetRegistry;
        static AssetsChangeEventFn sAssetsChangeCallback;
    };
}