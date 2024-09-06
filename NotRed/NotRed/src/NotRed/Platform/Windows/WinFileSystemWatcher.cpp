#include "nrpch.h"
#include "NotRed/Util/FileSystemWatcher.h"

#include <Windows.h>
#include <filesystem>

namespace NR
{
	FileSystemWatcher::FileSystemChangedCallbackFn FileSystemWatcher::sCallback;

	static bool sWatching = true;
	static HANDLE sWatcherThread;

	void FileSystemWatcher::SetChangeCallback(const FileSystemChangedCallbackFn& callback)
	{
		sCallback = callback;
	}

	static std::string wchar_to_string(wchar_t* input)
	{
		std::wstring string_input(input);
		std::string converted(string_input.begin(), string_input.end());
		return converted;
	}

	void FileSystemWatcher::StartWatching()
	{
		DWORD threadID;
		sWatcherThread = CreateThread(NULL, 0, Watch, 0, 0, &threadID);
		NR_CORE_ASSERT(sWatcherThread != NULL);
	}

	void FileSystemWatcher::StopWatching()
	{
		sWatching = false;
		DWORD result = WaitForSingleObject(sWatcherThread, 5000);
		if (result == WAIT_TIMEOUT)
		{
			TerminateThread(sWatcherThread, 0);
		}
		CloseHandle(sWatcherThread);
	}

	unsigned long FileSystemWatcher::Watch(void* param)
	{
		LPCWSTR	filepath = L"Assets";
		char* buffer = new char[1024];
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
				1024,
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

			for (;;)
			{
				FILE_NOTIFY_INFORMATION& fni = (FILE_NOTIFY_INFORMATION&)*buffer;
				std::filesystem::path filepath = "Assets/" + wchar_to_string(fni.FileName);

				FileSystemChangedEvent e;
				e.Filepath = filepath.string();
				e.NewName = filepath.filename().string();
				e.OldName = filepath.filename().string();
				e.IsDirectory = std::filesystem::is_directory(filepath);

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
					ZeroMemory(buffer, 1024);
					break;
				}

				buffer += fni.NextEntryOffset;
			}
		}

		return 0;
	}
}