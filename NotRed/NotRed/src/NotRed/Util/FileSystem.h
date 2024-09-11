#pragma once

#include <functional>

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
		std::string FilePath;
		std::string OldName;
		std::string NewName;
		bool IsDirectory;
	};

	class FileSystem
	{
	public:
		using FileSystemChangedCallbackFn = std::function<void(FileSystemChangedEvent)>;

	public:
		static bool CreateFolder(const std::string& filepath);
		static bool Exists(const std::string& filePath);
		static std::string Rename(const std::string& filepath, const std::string& newName);
		static bool DeleteFile(const std::string& filepath);
		static bool MoveFile(const std::string& filepath, const std::string& dest);

		static void SetChangeCallback(const FileSystemChangedCallbackFn& callback);
		static void StartWatching();
		static void StopWatching();

	private:
		static unsigned long Watch(void* param);

	private:
		static FileSystemChangedCallbackFn sCallback;
	};

}