#include <NotRed.h>
#include <NotRed/EntryPoint.h>

#include <Shlobj.h>

#include "EditorLayer.h"

class NotEditorApplication : public NR::Application
{
public:
	NotEditorApplication(const NR::ApplicationSpecification& specification, std::string_view projectPath)
		: Application(specification), mProjectPath(projectPath), mUserPreferences(NR::Ref<NR::UserPreferences>::Create())
	{
		if (projectPath.empty())
		{
			mProjectPath = "SandboxProject/Sandbox.nrproj";
		}
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

			if (!mProjectPath.empty())
			{
				mUserPreferences->StartupProject = mProjectPath;
			}
			else if (!mUserPreferences->StartupProject.empty())
			{
				mProjectPath = mUserPreferences->StartupProject;
			}
		}

		NR::EditorLayer* editorLayer = new NR::EditorLayer(mUserPreferences);
		PushLayer(editorLayer);

		if (std::filesystem::exists(mProjectPath))
		{
			editorLayer->OpenProject(mProjectPath);
		}
	}

private:
	std::string mProjectPath;
	std::filesystem::path mPersistentStoragePath;
	NR::Ref<NR::UserPreferences> mUserPreferences;
};

NR::Application* NR::CreateApplication(int argc, char** argv)
{
	std::string_view projectPath;
	if (argc > 1)
	{
		projectPath = argv[1];
	}

	NR::ApplicationSpecification specification;
	specification.Name = "NotEditor";
	specification.WindowWidth = 1600;
	specification.WindowHeight = 900;
	specification.StartMaximized = true;
	specification.VSync = true;
	return new NotEditorApplication(specification, projectPath);
}