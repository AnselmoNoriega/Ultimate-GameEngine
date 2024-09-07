#include "nrpch.h"
#include "NotRed/Util/FileSystem.h"

#include <Windows.h>
#include <filesystem>

#include "NotRed/Util/AssetManager.h"

namespace NR
{
	FileSystem::FileSystemChangedCallbackFn FileSystem::sCallback;

	static bool sWatching = true;
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

	unsigned long FileSystem::Watch(void* param)
	{
		LPCWSTR	filepath = L"Assets";
		BYTE buffer[1024];
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
				sizeof(buffer),
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

			std::string oldName;

			char fileName[MAX_PATH] = "";

			BYTE* current = buffer;

			for (;;)
			{
				ZeroMemory(fileName, sizeof(fileName));

				FILE_NOTIFY_INFORMATION* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(current);
				WideCharToMultiByte(CP_ACP, 0, fni->FileName, fni->FileNameLength / sizeof(WCHAR), fileName, sizeof(fileName), NULL, NULL);
				std::filesystem::path filepath = "assets/" + std::string(fileName);

				FileSystemChangedEvent e;
				e.FilePath = filepath.string();
				e.NewName = filepath.filename().string();
				e.OldName = filepath.filename().string();
				e.IsDirectory = std::filesystem::is_directory(filepath);

				switch (fni->Action)
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

				if (!fni->NextEntryOffset)
				{
					ZeroMemory(buffer, 1024);
					break;
				}

				current += fni->NextEntryOffset;
			}
		}

		return 0;
	}
}