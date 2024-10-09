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
        using AssetsChangeEventFn = std::function<void()>;

        struct AssetMetadata
        {
            AssetHandle Handle;
            std::string FilePath;
            AssetType Type;
        };

    public:
        static void Init();
        static void Shutdown();

        static void SetAssetChangeCallback(const AssetsChangeEventFn& callback);
        static std::vector<Ref<Asset>> GetAssetsInDirectory(AssetHandle directoryHandle);

        static std::vector<Ref<Asset>> SearchAssets(const std::string& query, const std::string& searchPath, AssetType desiredTypes = AssetType::None);

        static bool IsDirectory(const std::string& filepath);

        static AssetHandle GetAssetHandleFromFilePath(const std::string& filepath);
        static bool IsAssetHandleValid(AssetHandle assetHandle) { return assetHandle != 0 && sLoadedAssets.find(assetHandle) != sLoadedAssets.end(); }

        static void Rename(AssetHandle assetHandle, const std::string& newName);
        static void MoveAsset(AssetHandle assetHandle, AssetHandle newDirectory);
        static void RemoveAsset(AssetHandle assetHandle);

        static AssetType GetAssetTypeForFileType(const std::string& extension);

        template<typename T, typename... Args>
        static Ref<T> CreateNewAsset(const std::string& filename, AssetType type, AssetHandle directoryHandle, Args&&... args)
        {
            static_assert(std::is_base_of<Asset, T>::value, "CreateNewAsset only works for types derived from Asset");

            const auto& directory = GetAsset<Directory>(directoryHandle);

            Ref<T> asset = Ref<T>::Create(std::forward<Args>(args)...);
            asset->Type = type;
            asset->FilePath = directory->FilePath + "/" + filename;
            asset->FileName = Utils::RemoveExtension(Utils::GetFilename(asset->FilePath));
            asset->Extension = Utils::GetFilename(filename);
            asset->ParentDirectory = directoryHandle;
            asset->Handle = AssetHandle();
            asset->IsDataLoaded = true;
            sLoadedAssets[asset->Handle] = asset;
            AssetImporter::Serialize(asset);

            AssetMetadata metadata;
            metadata.Handle = asset->Handle;
            metadata.FilePath = asset->FilePath;
            metadata.Type = asset->Type;
            sAssetRegistry[asset->FilePath] = metadata;
            WriteRegistryToFile();

            return asset;
        }

        template<typename T>
        static Ref<T> GetAsset(AssetHandle assetHandle, bool loadData = true)
        {
            NR_CORE_ASSERT(sLoadedAssets.find(assetHandle) != sLoadedAssets.end());
            Ref<Asset>& asset = sLoadedAssets[assetHandle];

            if (!asset->IsDataLoaded && loadData)
            {
                AssetImporter::TryLoadData(asset);
            }

            return asset.As<T>();
        }

        template<typename T>
        static Ref<T> GetAsset(const std::string& filepath, bool loadData = true)
        {
            return GetAsset<T>(GetAssetHandleFromFilePath(filepath), loadData);
        }

    private:
        static void LoadAssetRegistry();
        static Ref<Asset> CreateAsset(const std::string& filepath, AssetType type, AssetHandle parentHandle);
        static void ImportAsset(const std::string& filepath, AssetHandle parentHandle);
        static AssetHandle ProcessDirectory(const std::string& directoryPath, AssetHandle parentHandle);
        static void ReloadAssets();
        static void WriteRegistryToFile();

        static void FileSystemChanged(FileSystemChangedEvent e);

        static AssetHandle FindParentHandleInChildren(Ref<Directory>& dir, const std::string& dirName);
        static AssetHandle FindParentHandle(const std::string& filepath);

    private:
        static std::unordered_map<AssetHandle, Ref<Asset>> sLoadedAssets;
        static std::unordered_map<std::string, AssetMetadata> sAssetRegistry;
        static AssetsChangeEventFn sAssetsChangeCallback;
    };
}