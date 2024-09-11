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

	bool FileSystem::CreateFolder(const std::string& filepath)
	{
		BOOL created = CreateDirectoryA(filepath.c_str(), NULL);
		if (!created)
		{
			DWORD error = GetLastError();

			if (error == ERROR_ALREADY_EXISTS)
			{
				NR_CORE_ERROR("{0} already exists!", filepath);
			}

			if (error == ERROR_PATH_NOT_FOUND)
			{
				NR_CORE_ERROR("{0}: One or more directories don't exist.", filepath);
			}

			return false;
		}

		return true;
	}

	bool FileSystem::Exists(const std::string& filepath)
	{
		DWORD attribs = GetFileAttributesA(filepath.c_str());

		if (attribs == INVALID_FILE_ATTRIBUTES)
		{
			return false;
		}

		return true;
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
		BOOL result = MoveFileA(filepath.c_str(), destFilePath.c_str());
		sIgnoreNextChange = false;
		return result != 0;
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
		DWORD threadID;
		sWatcherThread = CreateThread(NULL, 0, Watch, 0, 0, &threadID);
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

	static std::string wchar_to_string(wchar_t* input)
	{
		std::wstring string_input(input);
		std::string converted(string_input.begin(), string_input.end());
		return converted;
	}

	unsigned long FileSystem::Watch(void* param)
	{
		LPCWSTR	filepath = L"Assets";
		BYTE* buffer = new BYTE[10 * 1024];
		OVERLAPPED overlapped = { 0 };
		HANDLE handle = NULL;
		DWORD bytesReturned = 0;

		ZeroMemory(buffer, 1024);

		handle = CreateFile(
			filepath,
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
			DWORD status = ReadDirectoryChangesW(
				handle,
				buffer,
				10 * 1024 * sizeof(BYTE),
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
				continue;
			}

			std::string oldName;
			char fileName[MAX_PATH * 10] = "";

			FILE_NOTIFY_INFORMATION* current = (FILE_NOTIFY_INFORMATION*)buffer;

			for (;;)
			{
				ZeroMemory(fileName, sizeof(fileName));
				WideCharToMultiByte(CP_ACP, 0, current->FileName, current->FileNameLength / sizeof(WCHAR), fileName, sizeof(fileName), NULL, NULL);
				std::filesystem::path filepath = "assets/" + std::string(fileName);

				FileSystemChangedEvent e;
				e.FilePath = filepath.string();
				e.NewName = filepath.filename().string();
				e.OldName = filepath.filename().string();
				e.IsDirectory = std::filesystem::is_directory(filepath);

				switch (current->Action)
				{
				case FILE_ACTION_ADDED:
				{
					e.Action = FileSystemAction::Added;
					sCallback(e);
					break;
				}
				case FILE_ACTION_REMOVED:
				{
					e.IsDirectory = AssetManager::IsDirectory(e.FilePath);
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

				if (!current->NextEntryOffset)
				{
					ZeroMemory(buffer, 1024);
					break;
				}

				current += current->NextEntryOffset;
			}
		}

		return 0;
	}
}