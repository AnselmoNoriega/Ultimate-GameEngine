#include "nrpch.h"
#include "AssetManager.h"

#include <filesystem>

#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Renderer/SceneRenderer.h"
#include "NotRed/Project/Project.h"
#include "NotRed/ImGui/ImGui.h"

#include "yaml-cpp/yaml.h"

#include "AssetExtensions.h"

namespace NR
{
	void AssetManager::Init()
	{
		sAssetRegistry.Clear();
		AssetImporter::Init();

		LoadAssetRegistry();
		FileSystem::SetChangeCallback(AssetManager::FileSystemChanged);
		ReloadAssets();
	}

	void AssetManager::SetAssetChangeCallback(const AssetsChangeEventFn& callback)
	{
		sAssetsChangeCallback = callback;
	}

	void AssetManager::Shutdown()
	{
		WriteRegistryToFile();

		sMemoryAssets.clear();
		sAssetRegistry.Clear();
		sLoadedAssets.clear();
	}

	void AssetManager::FileSystemChanged(const std::vector<FileSystemChangedEvent>& events)
	{
		// Process all events before the refreshing the Content Browser
		for (const auto& e : events)
		{
			if (!e.IsDirectory)
			{
				switch (e.Action)
				{
				case FileSystemAction::Modified:
				{
					AssetHandle handle = GetAssetHandleFromFilePath(e.FilePath);
					const auto& metadata = GetMetadata(handle);

					if (metadata.Type == AssetType::Prefab)
						break;

					if (metadata.IsValid() && metadata.IsDataLoaded)
						ReloadData(handle);
					break;
				}
				case FileSystemAction::Rename:
				{
					AssetType previousType = GetAssetTypeFromPath(e.OldName);
					AssetType newType = GetAssetTypeFromPath(e.FilePath);

					if (previousType == AssetType::None && newType != AssetType::None)
						ImportAsset(e.FilePath);
					else
						AssetRenamed(GetAssetHandleFromFilePath(e.FilePath.parent_path() / e.OldName), e.FilePath);
					break;
				}
				case FileSystemAction::Delete:
					AssetDeleted(GetAssetHandleFromFilePath(e.FilePath));
					break;
				}
			}
		}

		sAssetsChangeCallback(events);
	}

	static AssetMetadata sNullMetadata;
	AssetMetadata& AssetManager::GetMetadataInternal(AssetHandle handle)
	{
		for (auto& [filepath, metadata] : sAssetRegistry)
		{
			if (metadata.Handle == handle)
				return metadata;
		}

		return sNullMetadata;
	}

	const AssetMetadata& AssetManager::GetMetadata(AssetHandle handle)
	{
		return GetMetadataInternal(handle);
	}

	const AssetMetadata& AssetManager::GetMetadata(const std::filesystem::path& filepath)
	{
		if (sAssetRegistry.Contains(filepath))
			return sAssetRegistry[filepath];

		return sNullMetadata;
	}

	std::filesystem::path AssetManager::GetRelativePath(const std::filesystem::path& filepath)
	{
		std::string temp = filepath.string();
		if (temp.find(Project::GetActive()->GetAssetDirectory().string()) != std::string::npos)
			return std::filesystem::relative(filepath, Project::GetActive()->GetAssetDirectory());
		return filepath;
	}

	AssetHandle AssetManager::GetAssetHandleFromFilePath(const std::filesystem::path& filepath)
	{
		return sAssetRegistry.Contains(filepath) ? sAssetRegistry[filepath].Handle : AssetHandle(0);
	}

	void AssetManager::AssetRenamed(AssetHandle assetHandle, const std::filesystem::path& newFilePath)
	{
		AssetMetadata metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
			return;

		sAssetRegistry.Remove(metadata.FilePath);
		metadata.FilePath = sAssetRegistry.GetPathKey(newFilePath);
		sAssetRegistry[metadata.FilePath] = metadata;
		WriteRegistryToFile();
	}

	void AssetManager::AssetMoved(AssetHandle assetHandle, const std::filesystem::path& destinationPath)
	{
		AssetMetadata metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
			return;

		sAssetRegistry.Remove(metadata.FilePath);
		metadata.FilePath = destinationPath / metadata.FilePath.filename();
		sAssetRegistry[metadata.FilePath] = metadata;

		WriteRegistryToFile();
	}

	void AssetManager::AssetDeleted(AssetHandle assetHandle)
	{
		AssetMetadata metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
			return;

		sAssetRegistry.Remove(metadata.FilePath);
		sLoadedAssets.erase(assetHandle);
		WriteRegistryToFile();
	}

	AssetType AssetManager::GetAssetTypeFromExtension(const std::string& extension)
	{
		std::string ext = Utils::String::ToLowerCopy(extension);
		if (sAssetExtensionMap.find(ext) == sAssetExtensionMap.end())
			return AssetType::None;

		return sAssetExtensionMap.at(ext.c_str());
	}

	AssetType AssetManager::GetAssetTypeFromPath(const std::filesystem::path& path)
	{
		return GetAssetTypeFromExtension(path.extension().string());
	}

	void AssetManager::LoadAssetRegistry()
	{
		NR_CORE_INFO("[AssetManager] Loading Asset Registry");

		const auto& assetRegistryPath = Project::GetAssetRegistryPath();
		if (!FileSystem::Exists(assetRegistryPath))
			return;

		std::ifstream stream(assetRegistryPath);
		NR_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		auto handles = data["Assets"];
		if (!handles)
		{
			NR_CORE_ERROR("[AssetManager] Asset Registry appears to be corrupted!");
			NR_CORE_VERIFY(false);
			return;
		}

		for (auto entry : handles)
		{
			std::string filepath = entry["FilePath"].as<std::string>();

			AssetMetadata metadata;
			metadata.Handle = entry["Handle"].as<uint64_t>();
			metadata.FilePath = filepath;
			metadata.Type = (AssetType)Utils::AssetTypeFromString(entry["Type"].as<std::string>());

			if (metadata.Type == AssetType::None)
				continue;

			if (!FileSystem::Exists(AssetManager::GetFileSystemPath(metadata)))
			{
				NR_CORE_WARN("[AssetManager] Missing asset '{0}' detected in registry file, trying to locate...", metadata.FilePath);

				std::string mostLikelyCandidate;
				uint32_t bestScore = 0;

				for (auto& pathEntry : std::filesystem::recursive_directory_iterator(Project::GetAssetDirectory()))
				{
					const std::filesystem::path& path = pathEntry.path();

					if (path.filename() != metadata.FilePath.filename())
						continue;

					if (bestScore > 0)
						NR_CORE_WARN("[AssetManager] Multiple candidates found...");

					std::vector<std::string> candiateParts = Utils::SplitString(path.string(), "/\\");

					uint32_t score = 0;
					for (const auto& part : candiateParts)
					{
						if (filepath.find(part) != std::string::npos)
							score++;
					}

					NR_CORE_WARN("'{0}' has a score of {1}, best score is {2}", path.string(), score, bestScore);

					if (bestScore > 0 && score == bestScore)
					{
						// TODO: How do we handle this?
						// Probably prompt the user at this point?
					}

					if (score <= bestScore)
						continue;

					bestScore = score;
					mostLikelyCandidate = path.string();
				}

				if (mostLikelyCandidate.empty() && bestScore == 0)
				{
					NR_CORE_ERROR("[AssetManager] Failed to locate a potential match for '{0}'", metadata.FilePath);
					continue;
				}

				std::replace(mostLikelyCandidate.begin(), mostLikelyCandidate.end(), '\\', '/');
				metadata.FilePath = std::filesystem::relative(mostLikelyCandidate, Project::GetActive()->GetAssetDirectory());
				NR_CORE_WARN("[AssetManager] Found most likely match '{0}'", metadata.FilePath);
			}

			if (metadata.Handle == 0)
			{
				NR_CORE_WARN("[AssetManager] AssetHandle for {0} is 0, this shouldn't happen.", metadata.FilePath);
				continue;
			}

			sAssetRegistry[metadata.FilePath] = metadata;
		}

		NR_CORE_INFO("[AssetManager] Loaded {0} asset entries", sAssetRegistry.Count());
	}

	AssetHandle AssetManager::ImportAsset(const std::filesystem::path& filepath)
	{
		std::filesystem::path path = GetRelativePath(filepath);

		if (sAssetRegistry.Contains(path))
			return sAssetRegistry[path].Handle;

		AssetType type = GetAssetTypeFromPath(path);
		if (type == AssetType::None)
			return 0;

		AssetMetadata metadata;
		metadata.Handle = AssetHandle();
		metadata.FilePath = path;
		metadata.Type = type;
		sAssetRegistry[metadata.FilePath] = metadata;

		return metadata.Handle;
	}

	bool AssetManager::ReloadData(AssetHandle assetHandle)
	{
		auto& metadata = GetMetadataInternal(assetHandle);
		if (!metadata.IsDataLoaded)
		{
			NR_CORE_WARN("Trying to reload asset that was never loaded");

			Ref<Asset> asset;
			metadata.IsDataLoaded = AssetImporter::TryLoadData(metadata, asset);
			return metadata.IsDataLoaded;
		}

		NR_CORE_ASSERT(sLoadedAssets.find(assetHandle) != sLoadedAssets.end());
		Ref<Asset>& asset = sLoadedAssets.at(assetHandle);
		metadata.IsDataLoaded = AssetImporter::TryLoadData(metadata, asset);
		return metadata.IsDataLoaded;
	}

	void AssetManager::ProcessDirectory(const std::filesystem::path& directoryPath)
	{
		for (auto entry : std::filesystem::directory_iterator(directoryPath))
		{
			if (entry.is_directory())
				ProcessDirectory(entry.path());
			else
				ImportAsset(entry.path());
		}
	}

	void AssetManager::ReloadAssets()
	{
		ProcessDirectory(Project::GetAssetDirectory().string());
		WriteRegistryToFile();
	}

	void AssetManager::WriteRegistryToFile()
	{
		// Sort assets by UUID to make project managment easier
		struct AssetRegistryEntry
		{
			std::string FilePath;
			AssetType Type;
		};
		std::map<UUID, AssetRegistryEntry> sortedMap;
		for (auto& [filepath, metadata] : sAssetRegistry)
		{
			if (!FileSystem::Exists(GetFileSystemPath(metadata)))
				continue;

			std::string pathToSerialize = metadata.FilePath.string();
			// NOTE(Yan): if Windows
			std::replace(pathToSerialize.begin(), pathToSerialize.end(), '\\', '/');
			sortedMap[metadata.Handle] = { pathToSerialize, metadata.Type };
		}

		NR_CORE_INFO("[AssetManager] serializing asset registry with {0} entries", sortedMap.size());

		YAML::Emitter out;
		out << YAML::BeginMap;

		out << YAML::Key << "Assets" << YAML::BeginSeq;
		for (auto& [handle, entry] : sortedMap)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Handle" << YAML::Value << handle;
			out << YAML::Key << "FilePath" << YAML::Value << entry.FilePath;
			out << YAML::Key << "Type" << YAML::Value << Utils::AssetTypeToString(entry.Type);
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		FileSystem::SkipNextFileSystemChange();

		const std::string& assetRegistryPath = Project::GetAssetRegistryPath().string();
		std::ofstream fout(assetRegistryPath);
		fout << out.c_str();
	}

	void AssetManager::ImGuiRender(bool& open)
	{
		if (!open)
			return;

		ImGui::Begin("Asset Manager", &open);
		if (UI::BeginTreeNode("Registry"))
		{
			static char searchBuffer[256];
			ImGui::InputText("##regsearch", searchBuffer, 256);
			UI::BeginPropertyGrid();
			static float columnWidth = 0.0f;
			if (columnWidth == 0.0f)
			{
				ImVec2 textSize = ImGui::CalcTextSize("File Path");
				columnWidth = textSize.x * 2.0f;
				ImGui::SetColumnWidth(0, columnWidth);
			}
			for (const auto& [path, metadata] : sAssetRegistry)
			{
				std::string handle = fmt::format("{0}", metadata.Handle);
				std::string filepath = metadata.FilePath.string();
				std::string type = Utils::AssetTypeToString(metadata.Type);
				if (searchBuffer[0] != 0)
				{
					std::string searchString = searchBuffer;
					Utils::String::ToLower(searchString);
					if (Utils::String::ToLowerCopy(handle).find(searchString) != std::string::npos
						|| Utils::String::ToLowerCopy(filepath).find(searchString) != std::string::npos
						|| Utils::String::ToLowerCopy(type).find(searchString) != std::string::npos)
					{
						UI::Property("Handle", (const std::string&)handle);
						UI::Property("File Path", (const std::string&)filepath);
						UI::Property("Type", (const std::string&)type);
						UI::Separator();
					}
				}
				else
				{
					UI::Property("Handle", (const std::string&)fmt::format("{0}", metadata.Handle));
					UI::Property("File Path", (const std::string&)metadata.FilePath.string());
					UI::Property("Type", (const std::string&)Utils::AssetTypeToString(metadata.Type));
					UI::Separator();
				}
			}
			UI::EndPropertyGrid();
			UI::EndTreeNode();
		}
		ImGui::End();
	}

	std::unordered_map<AssetHandle, Ref<Asset>> AssetManager::sLoadedAssets;
	std::unordered_map<AssetHandle, Ref<Asset>> AssetManager::sMemoryAssets;

	AssetManager::AssetsChangeEventFn AssetManager::sAssetsChangeCallback;

}