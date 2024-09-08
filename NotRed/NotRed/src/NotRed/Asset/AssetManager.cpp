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
        sTypes["hpm"] = AssetType::PhysicsMat;
        sTypes["png"] = AssetType::Texture;
        sTypes["hdr"] = AssetType::EnvMap;
        sTypes["wav"] = AssetType::Audio;
        sTypes["ogg"] = AssetType::Audio;
        sTypes["cs"] = AssetType::Script;
    }

    size_t AssetTypes::GetAssetTypeID(const std::string& extension)
    {
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
        sDirectories.clear();
    }

    void AssetManager::SetAssetChangeCallback(const AssetsChangeEventFn& callback)
    {
        sAssetsChangeCallback = callback;
    }

    DirectoryInfo& AssetManager::GetDirectoryInfo(int index)
    {
        NR_CORE_ASSERT(index >= 0 && index < sDirectories.size());
        return sDirectories[index];
    }

    std::vector<Ref<Asset>> AssetManager::GetAssetsInDirectory(int dirIndex)
    {
        std::vector<Ref<Asset>> results;

        for (auto& asset : sLoadedAssets)
        {
            if (asset.second->ParentDirectory == dirIndex)
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

        int parentIndex = FindParentIndex(e.FilePath);

        if (e.Action == FileSystemAction::Added)
        {
            if (e.IsDirectory)
            {
                ProcessDirectory(e.FilePath, parentIndex);
            }
            else
            {
                ImportAsset(e.FilePath, false, parentIndex);
            }
        }

        if (e.Action == FileSystemAction::Modified)
        {
            if (!e.IsDirectory)
            {
                ImportAsset(e.FilePath, true, parentIndex);
            }
        }

        if (e.Action == FileSystemAction::Rename)
        {
            if (e.IsDirectory)
            {
                for (auto& dir : sDirectories)
                {
                    if (dir.DirectoryName == e.OldName)
                    {
                        dir.FilePath = e.FilePath;
                        dir.DirectoryName = e.NewName;
                    }
                }
            }
            else
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
        }

        if (e.Action == FileSystemAction::Delete)
        {
            if (e.IsDirectory)
            {
            }
            else
            {
                for (auto it = sLoadedAssets.begin(); it != sLoadedAssets.end(); ++it)
                {
                    if (it->second->FilePath != e.FilePath)
                    {
                        continue;
                    }

                    sLoadedAssets.erase(it);
                    break;
                }
            }
        }

        sAssetsChangeCallback();
    }

    SearchResults AssetManager::SearchFiles(const std::string& query, const std::string& searchPath)
    {
        SearchResults results;

        if (!searchPath.empty())
        {
            for (const auto& dir : sDirectories)
            {
                if (dir.DirectoryName.find(query) != std::string::npos && dir.FilePath.find(searchPath) != std::string::npos)
                {
                    results.Directories.push_back(dir);
                }
            }

            for (const auto& [key, asset] : sLoadedAssets)
            {
                if (asset->FileName.find(query) != std::string::npos && asset->FilePath.find(searchPath) != std::string::npos)
                {
                    results.Assets.push_back(asset);
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

    int AssetManager::FindParentIndexInChildren(DirectoryInfo& dir, const std::string& dirName)
    {
        if (dir.DirectoryName == dirName)
        {
            return dir.DirectoryIndex;
        }

        for (int childIndex : dir.ChildrenIndices)
        {
            DirectoryInfo& child = AssetManager::GetDirectoryInfo(childIndex);

            int dirIndex = FindParentIndexInChildren(child, dirName);

            if (dirIndex != 0)
            {
                return dirIndex;
            }
        }

        return 0;
    }

    int AssetManager::FindParentIndex(const std::string& filepath)
    {
        std::vector<std::string> parts = Utils::SplitString(filepath, "/\\");
        std::string parentFolder = parts[parts.size() - 2];
        DirectoryInfo& assetsDirectory = GetDirectoryInfo(0);
        return FindParentIndexInChildren(assetsDirectory, parentFolder);
    }

    std::string AssetManager::GetParentPath(const std::string& path)
    {
        return std::filesystem::path(path).parent_path().string();
    }

    bool AssetManager::IsDirectory(const std::string& filepath)
    {
        for (auto& dir : sDirectories)
        {
            if (dir.FilePath == filepath)
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

    void AssetManager::ImportAsset(const std::string& filepath, bool reimport, int parentIndex)
    {
        std::string extension = Utils::GetExtension(filepath);
        if (extension == "meta")
        {
            return;
        }

        AssetType type = AssetTypes::GetAssetTypeFromExtension(extension);

        Ref<Asset> asset = AssetSerializer::Deserialize(filepath, parentIndex, reimport, type);
        sLoadedAssets[asset->Handle] = asset;
    }

    //TODO: This takes toooooooo long
    int AssetManager::ProcessDirectory(const std::string& directoryPath, int parentIndex)
    {
        DirectoryInfo dirInfo;
        dirInfo.DirectoryName = std::filesystem::path(directoryPath).filename().string();
        dirInfo.ParentIndex = parentIndex;
        dirInfo.FilePath = directoryPath;
        sDirectories.push_back(dirInfo);
        int currentIndex = sDirectories.size() - 1;
        sDirectories[currentIndex].DirectoryIndex = currentIndex;

        if (parentIndex != -1)
        {
            sDirectories[parentIndex].ChildrenIndices.push_back(currentIndex);
        }

        for (auto entry : std::filesystem::directory_iterator(directoryPath))
        {
            if (entry.is_directory())
            {
                ProcessDirectory(entry.path().string(), currentIndex);
            }
            else
            {
                ImportAsset(entry.path().string(), false, currentIndex);
            }
        }

        return dirInfo.DirectoryIndex;
    }

    void AssetManager::ReloadAssets()
    {
        ProcessDirectory("assets");
    }

    std::map<std::string, AssetType> AssetTypes::sTypes;

    std::unordered_map<AssetHandle, Ref<Asset>> AssetManager::sLoadedAssets;
    std::vector<DirectoryInfo> AssetManager::sDirectories;
    AssetManager::AssetsChangeEventFn AssetManager::sAssetsChangeCallback;
}