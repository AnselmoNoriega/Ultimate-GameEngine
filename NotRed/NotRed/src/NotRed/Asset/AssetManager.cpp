#include "nrpch.h"
#include "AssetManager.h"

#include <filesystem>

#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Renderer/SceneRenderer.h"

#include "yaml-cpp/yaml.h"

namespace NR
{
    void AssetTypes::Init()
    {
        sTypes["hsc"] = AssetType::Scene;
        sTypes["fbx"] = AssetType::Mesh;
        sTypes["obj"] = AssetType::Mesh;
        sTypes["blend"] = AssetType::Mesh;
        sTypes["nrpm"] = AssetType::PhysicsMat;
        sTypes["png"] = AssetType::Texture;
        sTypes["hdr"] = AssetType::EnvMap;
        sTypes["wav"] = AssetType::Audio;
        sTypes["ogg"] = AssetType::Audio;
        sTypes["cs"] = AssetType::Script;
    }

    size_t AssetTypes::GetAssetTypeID(const std::string& extension)
    {
        if (extension == "")
        {
            return 0;
        }

        for (auto& kv : sTypes)
        {
            if (kv.first == extension)
            {
                return std::hash<std::string>()(extension);
            }
        }

        return -1;
    }

    AssetType AssetTypes::GetAssetTypeFromExtension(const std::string& extension)
    {
        return sTypes.find(extension) != sTypes.end() ? sTypes[extension] : AssetType::Other;
    }

    void AssetManager::Init()
    {
        FileSystem::SetChangeCallback(AssetManager::FileSystemChanged);
        ReloadAssets();
    }

    void AssetManager::Shutdown()
    {
        sLoadedAssets.clear();
    }

    void AssetManager::SetAssetChangeCallback(const AssetsChangeEventFn& callback)
    {
        sAssetsChangeCallback = callback;
    }

    std::vector<Ref<Asset>> AssetManager::GetAssetsInDirectory(AssetHandle directoryHandle)
    {
        std::vector<Ref<Asset>> results;

        for (auto& asset : sLoadedAssets)
        {
            if (asset.second && asset.second->ParentDirectory == directoryHandle && asset.second->Handle != directoryHandle)
            {
                results.push_back(asset.second);
            }
        }

        return results;
    }

    void AssetManager::FileSystemChanged(FileSystemChangedEvent e)
    {
        e.NewName = Utils::RemoveExtension(e.NewName);
        e.OldName = Utils::RemoveExtension(e.OldName);

        AssetHandle parentHandle = FindParentHandle(e.FilePath);

        if (e.Action == FileSystemAction::Added)
        {
            if (e.IsDirectory)
            {
                ProcessDirectory(e.FilePath, parentHandle);
            }
            else
            {
                ImportAsset(e.FilePath, parentHandle);
            }
        }

        if (e.Action == FileSystemAction::Modified)
        {
            if (!e.IsDirectory)
            {
                ImportAsset(e.FilePath, parentHandle);
            }
        }

        if (e.Action == FileSystemAction::Rename)
        {
            for (auto it = sLoadedAssets.begin(); it != sLoadedAssets.end(); ++it)
            {
                if (it->second->FileName == e.OldName)
                {
                    it->second->FilePath = e.FilePath;
                    it->second->FileName = e.NewName;
                }
            }
        }

        if (e.Action == FileSystemAction::Delete)
        {
            for (auto it = sLoadedAssets.begin(); it != sLoadedAssets.end(); ++it)
            {
                if (it->second->FilePath != e.FilePath)
                {
                    continue;
                }

                RemoveAsset(it->first);
                break;
            }
        }

        sAssetsChangeCallback();
    }

    std::vector<Ref<Asset>> AssetManager::SearchFiles(const std::string& query, const std::string& searchPath)
    {
        std::vector<Ref<Asset>> results;

        if (!searchPath.empty())
        {
            for (const auto& [key, asset] : sLoadedAssets)
            {
                if (asset->FileName.find(query) != std::string::npos && asset->FilePath.find(searchPath) != std::string::npos)
                {
                    results.push_back(asset);
                }
            }
        }

        return results;
    }

    AssetHandle AssetManager::GetAssetIDForFile(const std::string& filepath)
    {
        for (auto& [id, asset] : sLoadedAssets)
        {
            if (asset->FilePath == filepath)
            {
                return id;
            }
        }

        return 0;
    }

    bool AssetManager::IsAssetHandleValid(AssetHandle assetHandle)
    {
        return sLoadedAssets.find(assetHandle) != sLoadedAssets.end();
    }

    void AssetManager::Rename(Ref<Asset>&asset, const std::string & newName)
    {
        std::string newFilePath = FileSystem::Rename(asset->FilePath, newName);
        std::string oldFilePath = asset->FilePath;
        asset->FilePath = newFilePath;
        asset->FileName = newName;

        if (FileSystem::Exists(oldFilePath + ".meta"))
        {
            std::string metaFileName = oldFilePath;

            if (asset->Extension != "")
            {
                metaFileName += "." + asset->Extension;
            }

            FileSystem::Rename(oldFilePath + ".meta", metaFileName);
            AssetSerializer::CreateMetaFile(asset);
        }
    }

    AssetHandle AssetManager::FindParentHandleInChildren(Ref<Directory>& dir, const std::string& dirName)
    {
        if (dir->FileName == dirName)
        {
            return dir->Handle;
        }

        for (AssetHandle childHandle : dir->ChildDirectories)
        {
            Ref<Directory> child = GetAsset<Directory>(childHandle);
            AssetHandle dirHandle = FindParentHandleInChildren(child, dirName);

            if (IsAssetHandleValid(dirHandle))
            {
                return dirHandle;
            }
        }

        return 0;
    }

    AssetHandle AssetManager::FindParentHandle(const std::string& filepath)
    {
        std::vector<std::string> parts = Utils::SplitString(filepath, "/\\");
        std::string parentFolder = parts[parts.size() - 2];
        Ref<Directory> assetsDirectory = GetAsset<Directory>(0);
        return FindParentHandleInChildren(assetsDirectory, parentFolder);
    }

    std::string AssetManager::GetParentPath(const std::string& path)
    {
        return std::filesystem::path(path).parent_path().string();
    }

    bool AssetManager::IsDirectory(const std::string& filepath)
    {
        for (auto& [handle, asset] : sLoadedAssets)
        {
            if (asset->Type == AssetType::Directory && asset->FilePath == filepath)
            {
                return true;
            }
        }

        return false;
    }

    std::string AssetManager::StripExtras(const std::string& filename)
    {
        std::vector<std::string> out;
        size_t start;
        size_t end = 0;

        while ((start = filename.find_first_not_of(".", end)) != std::string::npos)
        {
            end = filename.find(".", start);
            out.push_back(filename.substr(start, end - start));
        }

        if (out[0].length() >= 10)
        {
            auto cutFilename = out[0].substr(0, 9) + "...";
            return cutFilename;
        }

        auto filenameLength = out[0].length();
        auto paddingToAdd = 9 - filenameLength;

        std::string newFileName;

        for (int i = 0; i <= paddingToAdd; ++i)
        {
            newFileName += " ";
        }

        newFileName += out[0];

        return newFileName;
    }

    void AssetManager::RemoveAsset(AssetHandle assetHandle)
    {
        Ref<Asset> asset = sLoadedAssets[assetHandle];
        if (asset->Type == AssetType::Directory)
        {
            if (IsAssetHandleValid(asset->ParentDirectory))
            {
                auto& childList = sLoadedAssets[asset->ParentDirectory].As<Directory>()->ChildDirectories;
                childList.erase(std::remove(childList.begin(), childList.end(), assetHandle), childList.end());
            }

            for (auto child : asset.As<Directory>()->ChildDirectories)
            {
                RemoveAsset(child);
            }

            for (auto it = sLoadedAssets.begin(); it != sLoadedAssets.end(); )
            {
                if (it->second->ParentDirectory != assetHandle)
                {
                    ++it;
                    continue;
                }

                it = sLoadedAssets.erase(it);
            }
        }

        sLoadedAssets.erase(assetHandle);
    }

    void AssetManager::ImportAsset(const std::string& filepath, AssetHandle parentHandle)
    {
        std::string extension = Utils::GetExtension(filepath);
        if (extension == "meta")
        {
            return;
        }

        AssetType type = AssetTypes::GetAssetTypeFromExtension(extension);

        Ref<Asset> asset = AssetSerializer::LoadAssetInfo(filepath, parentHandle, type);

        if (sLoadedAssets.find(asset->Handle) != sLoadedAssets.end())
        {
            if (sLoadedAssets[asset->Handle]->IsDataLoaded)
            {
                asset = AssetSerializer::LoadAssetData(asset);
            }
        }

        sLoadedAssets[asset->Handle] = asset;
    }

    //TODO: This takes toooooooo long
    AssetHandle AssetManager::ProcessDirectory(const std::string& directoryPath, AssetHandle parentHandle)
    {
        Ref<Directory> dirInfo = AssetSerializer::LoadAssetInfo(directoryPath, parentHandle, AssetType::Directory).As<Directory>();
        sLoadedAssets[dirInfo->Handle] = dirInfo;

        if (parentHandle != dirInfo->Handle && IsAssetHandleValid(parentHandle))
        {
            sLoadedAssets[parentHandle].As<Directory>()->ChildDirectories.push_back(dirInfo->Handle);
        }

        for (auto entry : std::filesystem::directory_iterator(directoryPath))
        {
            if (entry.is_directory())
            {
                ProcessDirectory(entry.path().string(), dirInfo->Handle);
            }
            else
            {
                ImportAsset(entry.path().string(), dirInfo->Handle);
            }
        }

        return dirInfo->Handle;
    }

    void AssetManager::ReloadAssets()
    {
        ProcessDirectory("assets", 0);
    }

    std::map<std::string, AssetType> AssetTypes::sTypes;

    std::unordered_map<AssetHandle, Ref<Asset>> AssetManager::sLoadedAssets;
    AssetManager::AssetsChangeEventFn AssetManager::sAssetsChangeCallback;
}