#include "nrpch.h"
#include "AssetManager.h"

#include <filesystem>

namespace NR
{
    std::map<std::string, AssetType> AssetTypes::sTypes;

    AssetManager::AssetManager(const AssetsChangeEventFn& callback)
        : mAssetsChangeCallback(callback)
    {
        FileSystemWatcher::SetChangeCallback(NR_BIND_EVENT_FN(AssetManager::FileSystemChanged));
        mLoadedAssets = GetDirectoryContents("Assets", true);
    }

    void AssetTypes::Init()
    {
        sTypes["hsc"] = AssetType::Scene;
        sTypes["fbx"] = AssetType::Mesh;
        sTypes["obj"] = AssetType::Mesh;
        sTypes["png"] = AssetType::Image;
        sTypes["blend"] = AssetType::Mesh;
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

    std::string AssetManager::ParseFilename(const std::string& filepath, const char& delim)
    {
        std::vector<std::string> out;
        size_t start;
        size_t end = 0;

        while ((start = filepath.find_first_not_of(delim, end)) != std::string::npos)
        {
            end = filepath.find(delim, start);
            out.push_back(filepath.substr(start, end - start));
        }

        return out[out.size() - 1];
    }

    std::string AssetManager::ParseFileType(const std::string& filename)
    {
        size_t start;
        size_t end = 0;
        std::vector<std::string> out;

        while ((start = filename.find_first_not_of(".", end)) != std::string::npos)
        {
            end = filename.find(".", start);
            out.push_back(filename.substr(start, end - start));
        }

        return out[out.size() - 1];
    }

    void AssetManager::HandleAsset(const std::string& filepath)
    {

    }

    void AssetManager::ProcessAsset(const std::string& assetType)
    {
        std::string filename = ParseFilename(assetType, '/\\');
        std::string filetype = ParseFileType(assetType);
    }

    void AssetManager::FileSystemChanged(FileSystemChangedEvent e)
    {
        e.NewName = RemoveExtension(e.NewName);
        e.OldName = RemoveExtension(e.OldName);

        if (e.Action == FileSystemAction::Added)
        {
            mLoadedAssets = GetDirectoryContents("Assets", true);
        }

        if (e.Action == FileSystemAction::Modified)
        {
        }

        if (e.Action == FileSystemAction::Rename)
        {
            for (auto& entry : mLoadedAssets)
            {
                if (entry.Filename == e.OldName)
                {
                    entry.Filename = e.NewName;
                }
            }
        }

        if (e.Action == FileSystemAction::Delete)
        {
            for (auto it = mLoadedAssets.begin(); it != mLoadedAssets.end(); ++it)
            {
                if (it->Filename == e.NewName)
                {
                    mLoadedAssets.erase(it);
                    break;
                }
            }
        }

        mAssetsChangeCallback();
    }

    std::vector<DirectoryInfo> AssetManager::GetDirectoryContents(const std::string& filepath, bool recursive)
    {
        std::vector<DirectoryInfo> directories;

        if (recursive)
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(filepath))
            {
                bool isDir = std::filesystem::is_directory(entry);
                std::string dir_data = ParseFilename(entry.path().string(), '/\\');
                std::string fileExt = ParseFileType(dir_data);
                directories.emplace_back(dir_data, fileExt, entry.path().string(), !isDir);
            }
        }
        else
        {
            for (const auto& entry : std::filesystem::directory_iterator(filepath))
            {
                bool isDir = std::filesystem::is_directory(entry);
                std::string dir_data = ParseFilename(entry.path().string(), '/\\');
                std::string fileExt = ParseFileType(dir_data);
                directories.emplace_back(dir_data, fileExt, entry.path().string(), !isDir);
            }
        }

        return directories;
    }

    std::vector<DirectoryInfo> AssetManager::SearchFiles(const std::string& query, const std::string& searchPath)
    {
        std::vector<DirectoryInfo> result;

        if (!searchPath.empty())
        {
            for (const auto& entry : mLoadedAssets)
            {
                if (entry.Filename.find(query) != std::string::npos && entry.AbsolutePath.find(searchPath) != std::string::npos)
                {
                    result.push_back(entry);
                }
            }
        }

        return result;
    }

    std::string AssetManager::GetParentPath(const std::string& path)
    {
        return std::filesystem::path(path).parent_path().string();
    }

    std::vector<std::string> AssetManager::GetDirectoryNames(const std::string& filepath)
    {
        std::vector<std::string> result;
        size_t start;
        size_t end = 0;

        while ((start = filepath.find_first_not_of("/\\", end)) != std::string::npos)
        {
            end = filepath.find("/\\", start);
            result.push_back(filepath.substr(start, end - start));
        }

        return result;
    }

    bool AssetManager::MoveFile(const std::string& originalPath, const std::string& dest)
    {
        std::filesystem::rename(originalPath, dest);
        std::string newPath = dest + "/" + ParseFilename(originalPath, '/\\');
        return std::filesystem::exists(newPath);
    }

    std::string AssetManager::RemoveExtension(const std::string& filename)
    {
        std::string newName;
        size_t end = filename.find_last_of('.');

        newName = filename.substr(0, end);
        return newName;
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

    void AssetManager::ImportAsset(const std::string assetPath, const std::string& assetName)
    {

    }
}