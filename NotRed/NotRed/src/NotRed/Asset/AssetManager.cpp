#include "nrpch.h"
#include "AssetManager.h"

#include <filesystem>

#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Renderer/SceneRenderer.h"
#include "NotRed/Project/Project.h"

#include "yaml-cpp/yaml.h"

namespace NR
{
    void AssetManager::Init()
    {
        AssetImporter::Init();

        LoadAssetRegistry();
        FileSystem::SetChangeCallback(AssetManager::FileSystemChanged);
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

    void AssetManager::FileSystemChanged(FileSystemChangedEvent e)
    {
        e.FilePath = (Project::GetAssetDirectory() / e.FilePath).string();
        std::string temp = e.FilePath.string();
        std::replace(temp.begin(), temp.end(), '\\', '/');
        e.FilePath = temp;

        e.NewName = Utils::RemoveExtension(e.NewName);

        if (!e.IsDirectory)
        {
            switch (e.Action)
            {
            case FileSystemAction::Added:
            {
                ImportAsset(e.FilePath.string());
                break;
            }
            case FileSystemAction::Delete:
            {
                AssetDeleted(GetAssetHandleFromFilePath(e.FilePath.string()));
                break;
            }
            case FileSystemAction::Modified:
            {
                break;
            }
            case FileSystemAction::Rename:
            {
                AssetType previousType = GetAssetTypeForFileType(Utils::GetExtension(e.OldName));
                AssetType newType = GetAssetTypeForFileType(e.FilePath.stem().string());

                if (previousType == AssetType::None && newType != AssetType::None)
                {
                    ImportAsset(e.FilePath.string());
                }
                else
                {
                    AssetRenamed(GetAssetHandleFromFilePath((e.FilePath.parent_path() / e.OldName).string()), e.FilePath.string());
                    e.WasTracking = true;
                }
                break;
            }
            }
        }

        sAssetsChangeCallback(e);
    }

    static AssetMetadata sNullMetadata;

    AssetMetadata& AssetManager::GetMetadata(AssetHandle handle)
    {
        for (auto& [filepath, metadata] : sAssetRegistry)
        {
            if (metadata.Handle == handle)
            {
                return metadata;
            }
        }

        return sNullMetadata;
    }

    AssetMetadata& AssetManager::GetMetadata(const std::string& filepath)
    {
        std::string fixedFilePath = filepath;
        std::replace(fixedFilePath.begin(), fixedFilePath.end(), '\\', '/');

        if (sAssetRegistry.find(fixedFilePath) != sAssetRegistry.end())
        {
            return sAssetRegistry[fixedFilePath];
        }

        return sNullMetadata;
    }

    AssetHandle AssetManager::GetAssetHandleFromFilePath(const std::string& filePath)
    {
        std::string fixedFilePath = filePath;
        std::replace(fixedFilePath.begin(), fixedFilePath.end(), '\\', '/');

        if (sAssetRegistry.find(fixedFilePath) != sAssetRegistry.end())
        {
            return sAssetRegistry[fixedFilePath].Handle;
        }

        return 0;
    }

    std::string AssetManager::GetRelativePath(const std::string& filePath)
    {
        std::string result = filePath;
        if (filePath.find(Project::GetActive()->GetAssetDirectory().string()) != std::string::npos)
        {
            result = std::filesystem::relative(result, Project::GetActive()->GetAssetDirectory()).string();
        }

        std::replace(result.begin(), result.end(), '\\', '/');
        return result;
    }

    void AssetManager::AssetRenamed(AssetHandle assetHandle, const std::string& newFilePath)
    {
        AssetMetadata metadata = GetMetadata(assetHandle);
        sAssetRegistry.erase(metadata.FilePath.string());

        metadata.FilePath = newFilePath;
        metadata.FileName = Utils::RemoveExtension(Utils::GetFilename(newFilePath));

        sAssetRegistry[metadata.FilePath.string()] = metadata;
        WriteRegistryToFile();
    }

    void AssetManager::AssetMoved(AssetHandle assetHandle, const std::string& destinationPath)
    {
        AssetMetadata assetInfo = GetMetadata(assetHandle);

        sAssetRegistry.erase(assetInfo.FilePath.string());
        assetInfo.FilePath = destinationPath + "/" + assetInfo.FileName + "." + assetInfo.Extension;
        sAssetRegistry[assetInfo.FilePath.string()] = assetInfo;

        WriteRegistryToFile();
    }

    void AssetManager::AssetDeleted(AssetHandle assetHandle)
    {
        AssetMetadata metadata = GetMetadata(assetHandle);

        sAssetRegistry.erase(metadata.FilePath.string());
        sLoadedAssets.erase(assetHandle);
        
        WriteRegistryToFile();
    }

    AssetType AssetManager::GetAssetTypeForFileType(const std::string& extension)
    {
        if (extension == "nrsc") return AssetType::Scene;
        if (extension == "fbx") return AssetType::MeshAsset;
        if (extension == "obj") return AssetType::MeshAsset;
        if (extension == "nrm") return AssetType::Mesh;
        if (extension == "png") return AssetType::Texture;
        if (extension == "hdr") return AssetType::EnvMap;
        if (extension == "hpm") return AssetType::PhysicsMat;
        if (extension == "wav") return AssetType::Audio;
        if (extension == "ogg") return AssetType::Audio;
        return AssetType::None;
    }

    void AssetManager::LoadAssetRegistry()
    {
        const std::string& assetRegistryPath = Project::GetAssetRegistryPath().string();
        if (!FileSystem::Exists(assetRegistryPath))
        {
            return;
        }

        std::ifstream stream(assetRegistryPath);
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
            std::string filepath = entry["FilePath"].as<std::string>();
            if (filepath.find(Project::GetActive()->GetAssetDirectory().string()) == std::string::npos)
            {
                filepath = Project::GetActive()->GetAssetDirectory().string() + "/" + filepath;
            }

            std::replace(filepath.begin(), filepath.end(), '\\', '/');
            AssetMetadata metadata;

            metadata.Handle = entry["Handle"].as<uint64_t>();
            metadata.FilePath = filepath;
            metadata.FileName = Utils::RemoveExtension(metadata.FilePath.filename().string());
            metadata.Extension = Utils::GetExtension(metadata.FilePath.string());
            metadata.Type = (AssetType)Utils::AssetTypeFromString(entry["Type"].as<std::string>());

            if (metadata.Type == AssetType::None)
            {
                continue;
            }

            if (!FileSystem::Exists(AssetManager::GetFileSystemPath(metadata)))
            {
                NR_CORE_WARN("Missing asset '{0}' detected in registry file, trying to locate...", metadata.FilePath.string());
                
                std::string mostLikelyCandiate;
                uint32_t bestScore = 0;

                for (auto& pathEntry : std::filesystem::recursive_directory_iterator(Project::GetAssetDirectory()))
                {
                    const std::filesystem::path& path = pathEntry.path();
                    if (path.filename() != metadata.FilePath.filename())
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
                        if (filepath.find(part) != std::string::npos)
                        {
                            ++score;
                        }
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

                std::replace(mostLikelyCandiate.begin(), mostLikelyCandiate.end(), '\\', '/');
                metadata.FilePath = mostLikelyCandiate;
                NR_CORE_WARN("Found most likely match '{0}'", metadata.FilePath);
            }

            if (metadata.Handle == 0)
            {
                NR_CORE_WARN("AssetHandle for {0} is 0, this shouldn't happen.", metadata.FilePath);
                continue;
            }

            sAssetRegistry[metadata.FilePath.string()] = metadata;
        }
    }

    AssetHandle AssetManager::ImportAsset(const std::string& filepath)
    {
        std::string fixedFilePath = filepath;
        std::replace(fixedFilePath.begin(), fixedFilePath.end(), '\\', '/');

        // Already in the registry
        if (sAssetRegistry.find(fixedFilePath) != sAssetRegistry.end())
        {
            return 0;
        }

        AssetType type = GetAssetTypeForFileType(Utils::GetExtension(fixedFilePath));

        if (type == AssetType::None)
        {
            return 0;
        }

        AssetMetadata metadata;
        metadata.Handle = AssetHandle();
        metadata.FilePath = fixedFilePath;
        metadata.FileName = Utils::RemoveExtension(metadata.FilePath.filename().string());
        metadata.Extension = Utils::GetExtension(metadata.FilePath.string());
        metadata.Type = type;
        sAssetRegistry[metadata.FilePath.string()] = metadata;

        return metadata.Handle;
    }

    void AssetManager::ProcessDirectory(const std::string& directoryPath)
    {
        for (auto entry : std::filesystem::directory_iterator(directoryPath))
        {
            if (entry.is_directory())
            {
                ProcessDirectory(entry.path().string());
            }
            else
            {
                ImportAsset(entry.path().string());
            }
        }
    }

    void AssetManager::ReloadAssets()
    {
        ProcessDirectory(Project::GetAssetDirectory().string());

        WriteRegistryToFile();
    }

    void AssetManager::WriteRegistryToFile()
    {
        YAML::Emitter out;
        out << YAML::BeginMap;

        out << YAML::Key << "Assets" << YAML::BeginSeq;
        for (auto& [filepath, metadata] : sAssetRegistry)
        {
            std::string fp = std::filesystem::relative(metadata.FilePath, Project::GetAssetDirectory()).string();
            std::replace(fp.begin(), fp.end(), '\\', '/');

            out << YAML::BeginMap;
            out << YAML::Key << "Handle" << YAML::Value << metadata.Handle;
            out << YAML::Key << "FilePath" << YAML::Value << fp;
            out << YAML::Key << "Type" << YAML::Value << Utils::AssetTypeToString(metadata.Type);
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;

        const std::string& assetRegistryPath = Project::GetAssetRegistryPath().string();
        std::ofstream fout(assetRegistryPath);
        fout << out.c_str();
    }

    std::unordered_map<AssetHandle, Ref<Asset>> AssetManager::sLoadedAssets;
    std::unordered_map<std::string, AssetMetadata> AssetManager::sAssetRegistry;
    AssetManager::AssetsChangeEventFn AssetManager::sAssetsChangeCallback;
}