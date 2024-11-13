#pragma once

#include <filesystem>
#include <functional>

#include "NotRed/Core/Buffer.h"

namespace NR
{
	enum class FileSystemAction
	{
		Added, 
		Rename, 
		Modified, 
		Delete
	};

	struct FileSystemChangedEvent
	{
		FileSystemAction Action;
		std::filesystem::path FilePath;

		std::string OldName;
		std::string NewName;

		bool IsDirectory;
		bool WasTracking = false;
	};

	class FileSystem
	{
	public:
		using FileSystemChangedCallbackFn = std::function<void(FileSystemChangedEvent)>;

	public:
		static bool CreateDirectory(const std::filesystem::path& directory);
		static bool CreateDirectory(const std::string& directory);
		static bool Exists(const std::filesystem::path& filepath);

		static bool Exists(const std::string& filePath);
		static std::string Rename(const std::string& filepath, const std::string& newName);
		static bool DeleteFile(const std::string& filepath);
		static bool MoveFile(const std::string& filepath, const std::string& dest);

		static void SetChangeCallback(const FileSystemChangedCallbackFn& callback);
		static void StartWatching();
		static void StopWatching();
		static bool IsDirectory(const std::string& filepath);

		static bool ShowFileInExplorer(const std::filesystem::path& path);
		static bool OpenDirectoryInExplorer(const std::filesystem::path& path);
		static bool OpenExternally(const std::filesystem::path& path);

		static bool WriteBytes(const std::filesystem::path& filepath, const Buffer& buffer);
		static Buffer ReadBytes(const std::filesystem::path& filepath);

		static void SkipNextFileSystemChange();

	private:
		static unsigned long Watch(void* param);

	private:
		static FileSystemChangedCallbackFn sCallback;
	};
}