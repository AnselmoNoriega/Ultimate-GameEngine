#include "nrpch.h"
#include "NotRed/Util/FileSystem.h"

#include <Windows.h>
#include <filesystem>

#include "NotRed/Asset/AssetManager.h"

namespace NR
{

	FileSystem::FileSystemChangedCallbackFn FileSystem::sCallback;

	static bool sWatching = true;
	static bool sIgnoreNextChange = false;
	static HANDLE sWatcherThread;

	void FileSystem::SetChangeCallback(const FileSystemChangedCallbackFn& callback)
	{
		sCallback = callback;
	}

	std::string FileSystem::Rename(const std::string& filepath, const std::string& newName)
	{
		sIgnoreNextChange = true;

		std::filesystem::path p = filepath;
		std::string newFilePath = p.parent_path().string() + "/" + newName + p.extension().string();

		MoveFileA(filepath.c_str(), newFilePath.c_str());
		sIgnoreNextChange = false;
		
		return newFilePath;
	}

	bool FileSystem::MoveFile(const std::string& filepath, const std::string& dest)
	{
		sIgnoreNextChange = true;
		
		std::filesystem::path p = filepath;
		std::string destFilePath = dest + "/" + p.filename().string();
		
		BOOL result = MoveFileA(filepath.c_str(), destFilePath.c_str()) != 0;
		sIgnoreNextChange = false;
		
		return result;
	}

	bool FileSystem::DeleteFile(const std::string& filepath)
	{
		sIgnoreNextChange = true;
		
		std::string fp = filepath;
		fp.append(1, '\0');
		
		SHFILEOPSTRUCTA file_op;
		file_op.hwnd = NULL;
		file_op.wFunc = FO_DELETE;
		file_op.pFrom = fp.c_str();
		file_op.pTo = "";
		file_op.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
		file_op.fAnyOperationsAborted = false;
		file_op.hNameMappings = 0;
		file_op.lpszProgressTitle = "";

		int result = SHFileOperationA(&file_op);
		sIgnoreNextChange = false;
		
		return result == 0;
	}

	void FileSystem::StartWatching()
	{
		sWatching = true;
		DWORD threadId;
		sWatcherThread = CreateThread(NULL, 0, Watch, 0, 0, &threadId);
		NR_CORE_ASSERT(sWatcherThread != NULL);
	}

	void FileSystem::StopWatching()
	{
		sWatching = false;
		DWORD result = WaitForSingleObject(sWatcherThread, 5000);
		
		if (result == WAIT_TIMEOUT)
		{
			TerminateThread(sWatcherThread, 0);
		}
		
		CloseHandle(sWatcherThread);
	}

	bool FileSystem::IsDirectory(const std::string& filepath)
	{
		bool result = std::filesystem::is_directory(filepath);

		if (!result)
		{
			result = Utils::GetExtension(filepath).empty();
		}

		return result;
	}

	static std::string wchar_to_string(wchar_t* input)
	{
		return std::filesystem::path(input).string();
	}

	void FileSystem::SkipNextFileSystemChange()
	{
		sIgnoreNextChange = true;
	}

	unsigned long FileSystem::Watch(void* param)
	{
		std::string assetDirectory = Project::GetActive()->GetAssetDirectory().string();
		std::vector<BYTE> buffer;
		buffer.resize(10 * 1024);
		OVERLAPPED overlapped = { 0 };
		HANDLE handle = NULL;
		DWORD bytesReturned = 0;

		handle = CreateFileA(
			assetDirectory.c_str(),
			FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			NULL
		);

		ZeroMemory(&overlapped, sizeof(overlapped));

		if (handle == INVALID_HANDLE_VALUE)
		{
			NR_CORE_ERROR("Unable to accquire directory handle: {0}", GetLastError());
		}

		overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		if (overlapped.hEvent == NULL)
		{
			NR_CORE_ERROR("CreateEvent failed!");
			return 0;
		}

		while (sWatching)
		{
			const DWORD status = ReadDirectoryChangesW(
				handle,
				&buffer[0],
				(uint32_t)buffer.size(),
				TRUE,
				FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME,
				&bytesReturned,
				&overlapped,
				NULL
			);

			if (!status)
			{
				NR_CORE_ERROR(GetLastError());
			}

			DWORD waitOperation = WaitForSingleObject(overlapped.hEvent, 5000);
			if (waitOperation != WAIT_OBJECT_0)
			{
				continue;
			}

			if (sIgnoreNextChange)
			{
				sIgnoreNextChange = false;
				continue;
			}

			std::string oldName;
			char fileName[MAX_PATH * 10] = "";

			BYTE* buf = buffer.data();
			for (;;)
			{
				FILE_NOTIFY_INFORMATION& fni = *(FILE_NOTIFY_INFORMATION*)buf;
				ZeroMemory(fileName, sizeof(fileName));
				WideCharToMultiByte(CP_ACP, 0, fni.FileName, fni.FileNameLength / sizeof(WCHAR), fileName, sizeof(fileName), NULL, NULL);
				std::filesystem::path filepath = std::string(fileName);

				FileSystemChangedEvent e;
				e.FilePath = filepath;
				e.NewName = filepath.filename().string();
				e.OldName = filepath.filename().string();
				e.IsDirectory = IsDirectory(e.FilePath.string());

				switch (fni.Action)
				{
				case FILE_ACTION_ADDED:
				{
					e.Action = FileSystemAction::Added;
					sCallback(e);
					break;
				}
				case FILE_ACTION_REMOVED:
				{
					e.Action = FileSystemAction::Delete;
					sCallback(e);
					break;
				}
				case FILE_ACTION_MODIFIED:
				{
					e.Action = FileSystemAction::Modified;
					sCallback(e);
					break;
				}
				case FILE_ACTION_RENAMED_OLD_NAME:
				{
					oldName = filepath.filename().string();
					break;
				}
				case FILE_ACTION_RENAMED_NEW_NAME:
				{
					e.OldName = oldName;
					e.Action = FileSystemAction::Rename;
					sCallback(e);
					break;
				}
				}

				if (!fni.NextEntryOffset)
				{
					break;
				}

				buf += fni.NextEntryOffset;
			}
		}

		return 0;
	}

	bool FileSystem::WriteBytes(const std::filesystem::path& filepath, const Buffer& buffer)
	{
		std::ofstream stream(filepath, std::ios::binary | std::ios::trunc);

		if (!stream)
		{
			stream.close();
			return false;
		}

		stream.write((char*)buffer.Data, buffer.Size);
		stream.close();

		return true;
	}

	Buffer FileSystem::ReadBytes(const std::filesystem::path& filepath)
	{
		Buffer buffer;

		std::ifstream stream(filepath, std::ios::binary | std::ios::ate);
		NR_CORE_ASSERT(stream);

		std::streampos end = stream.tellg();
		stream.seekg(0, std::ios::beg);
		uint32_t size = end - stream.tellg();
		NR_CORE_ASSERT(size != 0);

		buffer.Allocate(size);
		stream.read((char*)buffer.Data, buffer.Size);

		return buffer;
	}

}