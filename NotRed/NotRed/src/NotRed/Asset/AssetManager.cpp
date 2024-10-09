#include "nrpch.h"
#include "AssetManager.h"

#include <filesystem>

#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Renderer/SceneRenderer.h"

#include "yaml-cpp/yaml.h"

namespace NR
{
    static const char* sAssetRegistryPath = "Assets/AssetRegistry.nrr";

    void AssetManager::Init()
    {
        AssetImporter::Init();

        LoadAssetRegistry();
        ReloadAssets();
        WriteRegistryToFile();
    }

    void AssetManager::Shutdown()
    {
        WriteRegistryToFile();

        sAssetRegistry.clear();
        sLoadedAssets.clear();
    }

    void AssetManager::SetAssetChangeCallback(const AssetsChangeEventFn& callback)
    {
        sAssetsChangeCallback = callback;
    }

    std::vector<Ref<Asset>> AssetManager::GetAssetsInDirectory(AssetHandle directoryHandle)
    {
        std::vector<Ref<Asset>> result;
        Ref<Directory> directory = sLoadedAssets[directoryHandle].As<Directory>();
        result.reserve(directory->ChildDirectories.size() + directory->Assets.size());
        
        for (auto& assetHandle : directory->ChildDirectories)
        {
            if (!IsAssetHandleValid(assetHandle))
            {
                continue;
            }

            result.push_back(sLoadedAssets[assetHandle]);
        }

        for (auto& assetHandle : directory->Assets)
        {
            if (!IsAssetHandleValid(assetHandle))
            {
                continue;
            }

            result.push_back(sLoadedAssets[assetHandle]);
        }

        return result;
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

    std::vector<Ref<Asset>> AssetManager::SearchAssets(const std::string& query, const std::string& searchPath, AssetType desiredType)
    {
        std::vector<Ref<Asset>> results;

        if (searchPath.empty())
        {
            return results;
        }

        std::string queryLowerCase = Utils::ToLower(query);
        Ref<Directory> searchOrigin = GetAsset<Directory>(searchPath);

        for (auto& childHandle : searchOrigin->ChildDirectories)
        {
            std::vector<Ref<Asset>> childResults = SearchAssets(query, sLoadedAssets[childHandle]->FilePath, desiredType);
            results.insert(results.end(), childResults.begin(), childResults.end());
        }

        for (auto& assetHandle : searchOrigin->Assets)
        {
            const auto& asset = sLoadedAssets[assetHandle];
            
            if (desiredType == AssetType::None && asset->Type == AssetType::Directory)
            {
                continue;
            }

            if (desiredType != AssetType::None && asset->Type != desiredType)
            {
                continue;
            }

            std::string filename = Utils::ToLower(asset->FileName);
            if (filename.find(queryLowerCase) != std::string::npos)
            {
                results.push_back(asset);
            }

            if (queryLowerCase[0] != '.')
            {
                continue;
            }

            if (asset->Extension.find(std::string(&queryLowerCase[1])) != std::string::npos)
            {
                results.push_back(asset);
            }
        }

        return results;
    }

    AssetHandle AssetManager::GetAssetHandleFromFilePath(const std::string& filePath)
    {
        std::string fixedFilePath = filePath;
        std::replace(fixedFilePath.begin(), fixedFilePath.end(), '\\', '/');
        for (auto& [id, asset] : sLoadedAssets)
        {
            if (asset->FilePath == fixedFilePath)
            {
                return id;
            }
        }

        return 0;
    }

    void AssetManager::Rename(AssetHandle assetHandle, const std::string& newName)
    {
        Ref<Asset>& asset = sLoadedAssets[assetHandle];
        AssetMetadata metadata = sAssetRegistry[asset->FilePath];
        std::string newFilePath = FileSystem::Rename(asset->FilePath, newName);
        asset->FilePath = newFilePath;
        asset->FileName = newName;

        sAssetRegistry.erase(metadata.FilePath);
        metadata.FilePath = newFilePath;

        sAssetRegistry[metadata.FilePath] = metadata;
        WriteRegistryToFile();
    }

    void AssetManager::MoveAsset(AssetHandle assetHandle, AssetHandle newDirectory)
    {
        Ref<Asset>& asset = sLoadedAssets[assetHandle];
        Ref<Directory> directory = sLoadedAssets[newDirectory].As<Directory>();
        Ref<Directory> currentDirectory = sLoadedAssets[asset->ParentDirectory].As<Directory>();

        AssetMetadata metadata = sAssetRegistry[asset->FilePath];
        bool result = FileSystem::MoveFile(asset->FilePath, directory->FilePath);
        if (!result)
        {
            return;
        }

        asset->FilePath = directory->FilePath + "/" + asset->FileName + "." + asset->Extension;
        sAssetRegistry.erase(metadata.FilePath);
        metadata.FilePath = asset->FilePath;

        sAssetRegistry[metadata.FilePath] = metadata;

        asset->ParentDirectory = currentDirectory->Handle;
        directory->Assets.push_back(assetHandle);
        currentDirectory->Assets.erase(std::remove(currentDirectory->Assets.begin(), currentDirectory->Assets.end(), assetHandle), currentDirectory->Assets.end());
        
        WriteRegistryToFile();
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
        Ref<Directory> assetsDirectory = GetAsset<Directory>(GetAssetHandleFromFilePath("Assets"));
        return FindParentHandleInChildren(assetsDirectory, parentFolder);
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

    void AssetManager::RemoveAsset(AssetHandle assetHandle)
    {
        Ref<Asset> asset = sLoadedAssets[assetHandle];
        if (asset->Type == AssetType::Directory)
        {
            Ref<Directory> dir = asset.As<Directory>();

            if (IsAssetHandleValid(asset->ParentDirectory))
            {
                auto& childList = sLoadedAssets[asset->ParentDirectory].As<Directory>()->ChildDirectories;
                childList.erase(std::remove(childList.begin(), childList.end(), assetHandle), childList.end());
            }

            for (auto child : dir->ChildDirectories)
            {
                RemoveAsset(child);
            }

            for (auto childHandle : dir->Assets)
            {
                auto child = sLoadedAssets[childHandle];
                sAssetRegistry.erase(child->FilePath);
                sLoadedAssets.erase(childHandle);
            }

            dir->ChildDirectories.clear();
            dir->Assets.clear();
        }
        else
        {
            auto parent = sLoadedAssets[asset->ParentDirectory].As<Directory>();
            parent->Assets.erase(std::remove(parent->Assets.begin(), parent->Assets.end(), assetHandle), parent->Assets.end());
        }

        sAssetRegistry.erase(asset->FilePath);
        sLoadedAssets.erase(assetHandle);
        
        WriteRegistryToFile();
    }

    AssetType AssetManager::GetAssetTypeForFileType(const std::string& extension)
    {
        if (extension == "hsc") return AssetType::Scene;
        if (extension == "fbx") return AssetType::Mesh;
        if (extension == "obj") return AssetType::Mesh;
        if (extension == "png") return AssetType::Texture;
        if (extension == "hdr") return AssetType::EnvMap;
        if (extension == "hpm") return AssetType::PhysicsMat;
        if (extension == "wav") return AssetType::Audio;
        if (extension == "ogg") return AssetType::Audio;
        if (extension == "cs")  return AssetType::Script;
        return AssetType::None;
    }

    void AssetManager::LoadAssetRegistry()
    {
        if (!FileSystem::Exists(sAssetRegistryPath))
        {
            return;
        }

        std::ifstream stream(sAssetRegistryPath);
        NR_CORE_ASSERT(stream);
        std::stringstream strStream;
        strStream << stream.rdbuf();

        YAML::Node data = YAML::Load(strStream.str());
        auto handles = data["Assets"];
        if (!handles)
        {
            NR_CORE_ERROR("AssetRegistry appears to be corrupted!");
            return;
        }

        for (auto entry : handles)
        {
            AssetMetadata metadata;
            metadata.Handle = entry["Handle"].as<uint64_t>();
            metadata.FilePath = entry["FilePath"].as<std::string>();
            metadata.Type = (AssetType)entry["Type"].as<int>();

            if (!FileSystem::Exists(metadata.FilePath))
            {
                NR_CORE_WARN("Missing asset '{0}' detected in registry file, trying to locate...", metadata.FilePath);
                
                std::string mostLikelyCandiate;
                uint32_t bestScore = 0;

                for (auto& pathEntry : std::filesystem::recursive_directory_iterator("Assets/"))
                {
                    const std::filesystem::path& path = pathEntry.path();
                    if (path.filename() != Utils::GetFilename(metadata.FilePath))
                    {
                        continue;
                    }

                    if (bestScore > 0)
                    {
                        NR_CORE_WARN("Multiple candiates found...");
                    }

                    std::vector<std::string> candiateParts = Utils::SplitString(path.string(), "/\\");
                    uint32_t score = 0;
                    for (const auto& part : candiateParts)
                    {
                        if (metadata.FilePath.find(part) != std::string::npos)
                            score++;
                    }

                    NR_CORE_WARN("'{0}' has a score of {1}, best score is {2}", path.string(), score, bestScore);
                    if (bestScore > 0 && score == bestScore)
                    {
                    }

                    if (score <= bestScore)
                    {
                        continue;
                    }

                    bestScore = score;
                    mostLikelyCandiate = path.string();
                }

                if (mostLikelyCandiate.empty() && bestScore == 0)
                {
                    NR_CORE_ERROR("Failed to locate a potential match for '{0}'", metadata.FilePath);
                    continue;
                }

                metadata.FilePath = mostLikelyCandiate;
                std::replace(metadata.FilePath.begin(), metadata.FilePath.end(), '\\', '/');
                NR_CORE_WARN("Found most likely match '{0}'", metadata.FilePath);
            }

            if (metadata.Handle == 0)
            {
                NR_CORE_WARN("AssetHandle for {0} is 0, this shouldn't happen.", metadata.FilePath);
                continue;
            }

            sAssetRegistry[metadata.FilePath] = metadata;
        }
    }

    Ref<Asset> AssetManager::CreateAsset(const std::string& filepath, AssetType type, AssetHandle parentHandle)
    {
        Ref<Asset> asset = Ref<Asset>::Create();

        if (type == AssetType::Directory)
        {
            asset = Ref<Directory>::Create();
        }

        std::string extension = Utils::GetExtension(filepath);
        asset->FilePath = filepath;
        std::replace(asset->FilePath.begin(), asset->FilePath.end(), '\\', '/');

        if (sAssetRegistry.find(asset->FilePath) != sAssetRegistry.end())
        {
            asset->Handle = sAssetRegistry[asset->FilePath].Handle;
            asset->Type = sAssetRegistry[asset->FilePath].Type;

            if (asset->Type != type)
            {
                NR_CORE_WARN("AssetType for '{0}' was different than the metadata. Did the file type change?", asset->FilePath);
                asset->Type = AssetType::None;
            }
        }
        else
        {
            asset->Handle = AssetHandle();
            asset->Type = type;
        }

        asset->FileName = Utils::RemoveExtension(Utils::GetFilename(asset->FilePath));
        asset->Extension = extension;
        asset->ParentDirectory = parentHandle;
        asset->IsDataLoaded = false;
        return asset;
    }

    void AssetManager::ImportAsset(const std::string& filepath, AssetHandle parentHandle)
    {
        std::string extension = Utils::GetExtension(filepath);
        AssetType type = GetAssetTypeForFileType(extension);
        Ref<Asset> asset = CreateAsset(filepath, type, parentHandle);

        if (asset->Type == AssetType::None)
        {
            return;
        }

        if (sAssetRegistry.find(asset->FilePath) == sAssetRegistry.end())
        {
            AssetMetadata metadata;
            metadata.Handle = asset->Handle;
            metadata.FilePath = asset->FilePath;
            metadata.Type = asset->Type;
            sAssetRegistry[asset->FilePath] = metadata;
        }

        sLoadedAssets[asset->Handle] = asset;

        Ref<Directory> directory = sLoadedAssets[parentHandle];
        directory->Assets.push_back(asset->Handle);
    }

    AssetHandle AssetManager::ProcessDirectory(const std::string& directoryPath, AssetHandle parentHandle)
    {
        Ref<Directory> dirInfo = CreateAsset(directoryPath, AssetType::Directory, parentHandle).As<Directory>();
        dirInfo->IsDataLoaded = true;

        if (sAssetRegistry.find(dirInfo->FilePath) == sAssetRegistry.end())
        {
            AssetMetadata metadata;
            metadata.Handle = dirInfo->Handle;
            metadata.FilePath = dirInfo->FilePath;
            metadata.Type = dirInfo->Type;
            sAssetRegistry[dirInfo->FilePath] = metadata;
        }

        sLoadedAssets[dirInfo->Handle] = dirInfo;

        if (IsAssetHandleValid(parentHandle))
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
        ProcessDirectory("Assets", 0);

        // Remove any non-existent assets from the asset registry
        for (auto it = sAssetRegistry.begin(); it != sAssetRegistry.end(); )
        {
            if (sLoadedAssets.find(it->second.Handle) == sLoadedAssets.end())
            {
                it = sAssetRegistry.erase(it);
            }
            else
            {
                ++it;
            }
        }

        WriteRegistryToFile();
    }

    void AssetManager::WriteRegistryToFile()
    {
        YAML::Emitter out;
        out << YAML::BeginMap;

        out << YAML::Key << "Assets" << YAML::BeginSeq;
        for (auto& [filepath, metadata] : sAssetRegistry)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "Handle" << YAML::Value << metadata.Handle;
            out << YAML::Key << "FilePath" << YAML::Value << metadata.FilePath;
            out << YAML::Key << "Type" << YAML::Value << (int)metadata.Type;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::ofstream fout(sAssetRegistryPath);
        fout << out.c_str();
    }

    std::unordered_map<AssetHandle, Ref<Asset>> AssetManager::sLoadedAssets;
    std::unordered_map<std::string, AssetManager::AssetMetadata> AssetManager::sAssetRegistry;
    AssetManager::AssetsChangeEventFn AssetManager::sAssetsChangeCallback;
}