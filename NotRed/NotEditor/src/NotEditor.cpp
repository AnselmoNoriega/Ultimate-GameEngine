#include <NotRed.h>
#include <NotRed/EntryPoint.h>

#include "EditorLayer.h"

class NotEditorApplication : public NR::Application
{
public:
	NotEditorApplication(const NR::ApplicationSpecification& specification, std::string_view projectPath)
		: Application(specification) ,mProjectPath(projectPath)
	{
	}

	void Init() override
	{
		NR::EditorLayer* editorLayer = new NR::EditorLayer();
		PushLayer(editorLayer);
		if (std::filesystem::exists(mProjectPath))
		{
			editorLayer->OpenProject(mProjectPath);
		}
	}

private:
	std::string mProjectPath;
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
	specification.VSync = true;
	return new NotEditorApplication(specification, projectPath);
}