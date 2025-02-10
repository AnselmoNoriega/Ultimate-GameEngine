#include <NotRed.h>
#include <NotRed/EntryPoint.h>
#include <NotRed/Util/FileSystem.h>

#include "LauncherLayer.h"

#include <Shlobj.h>

class NotEditorLauncherApplication : public NR::Application
{
public:
	NotEditorLauncherApplication(const NR::ApplicationSpecification& specification)
		: Application(specification), mUserPreferences(NR::Ref<NR::UserPreferences>::Create())
	{
	}

	void Init() override
	{
		// Persistent Storage
		{
			PWSTR roamingFilePath;
			HRESULT result = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, &roamingFilePath);
			NR_CORE_ASSERT(result == S_OK);

			std::wstring filepath = roamingFilePath;
			std::replace(filepath.begin(), filepath.end(), L'\\', L'/');
			mPersistentStoragePath = filepath + L"/NotEditor";

			if (!std::filesystem::exists(mPersistentStoragePath))
			{
				std::filesystem::create_directory(mPersistentStoragePath);
			}
		}

		// User Preferences
		{
			NR::UserPreferencesSerializer serializer(mUserPreferences);
			if (!std::filesystem::exists(mPersistentStoragePath / "UserPreferences.yaml"))
			{
				serializer.Serialize(mPersistentStoragePath / "UserPreferences.yaml");
			}
			else
			{
				serializer.Deserialize(mPersistentStoragePath / "UserPreferences.yaml");
			}
		}

		NR::LauncherProperties launcherProperties;
		launcherProperties.UserPreferences = mUserPreferences;
		launcherProperties.ProjectOpenedCallback = std::bind(&NotEditorLauncherApplication::ProjectOpened, this, std::placeholders::_1);

		// Installation Path
		{
			if (NR::FileSystem::HasEnvironmentVariable("NOTRED_DIR"))
				launcherProperties.InstallPath = NR::FileSystem::GetEnvironmentVariable("NOTRED_DIR");
		}

		SetShowStats(false);
		PushLayer(new NR::LauncherLayer(launcherProperties));
	}

private:
	void ProjectOpened(std::string projectPath)
	{
		std::filesystem::path notredDir = NR::FileSystem::GetEnvironmentVariable("NOTRED_DIR");
		std::string editorWorkingDirectory = (notredDir / "NotEditor").string();

#ifdef NR_DEBUG
		notredDir = notredDir / "bin" / "Debug-windows-x86_64" / "NotEditor";
#else
		notredDir = notredDir / "bin" / "Release-windows-x86_64" / "NotEditor";
#endif

		std::string editorExe = (notredDir / "NotEditor.exe").string();
		std::string commandLine = editorExe + " " + projectPath;

		PROCESS_INFORMATION processInfo;
		STARTUPINFOA startupInfo;
		ZeroMemory(&startupInfo, sizeof(STARTUPINFOA));
		startupInfo.cb = sizeof(startupInfo);

		bool result = CreateProcessA(editorExe.c_str(), commandLine.data(), NULL, NULL, FALSE, DETACHED_PROCESS, NULL, editorWorkingDirectory.c_str(), &startupInfo, &processInfo);
		if (result)
		{
			CloseHandle(processInfo.hThread);
			CloseHandle(processInfo.hProcess);
		}
	}

private:
	std::filesystem::path mPersistentStoragePath;
	NR::Ref<NR::UserPreferences> mUserPreferences;
};

NR::Application* NR::CreateApplication(int argc, char** argv)
{
	NR::ApplicationSpecification specification;
	specification.Name = "NotEditor Launcher";
	specification.WindowWidth = 1280;
	specification.WindowHeight = 720;
	specification.VSync = true;
	specification.StartMaximized = false;
	specification.Resizable = false;
	specification.WorkingDirectory = FileSystem::HasEnvironmentVariable("NOTRED_DIR") ? 
		FileSystem::GetEnvironmentVariable("NOTRED_DIR") + "/NotEditor" : 
		"../NotEditor";

	return new NotEditorLauncherApplication(specification);
}