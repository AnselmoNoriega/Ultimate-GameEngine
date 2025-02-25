#include <NotRed.h>
#include <NotRed/EntryPoint.h>

#include "RuntimeLayer.h"
#include "NotRed/Renderer/RendererAPI.h"

class RuntimeApplication : public NR::Application
{
public:
	RuntimeApplication(const NR::ApplicationSpecification& specification, std::string_view projectPath)
		: Application(specification) ,mProjectPath(projectPath)
	{
	}

	void Init() override
	{
		PushLayer(new NR::RuntimeLayer(mProjectPath));
	}

private:
	std::string mProjectPath;
};

NR::Application* NR::CreateApplication(int argc, char** argv)
{
	std::string_view projectPath = "SandboxProject/Sandbox.nrproj";
	if (argc > 1)
	{
		projectPath = argv[1];
	}

	NR::ApplicationSpecification specification;
	specification.Name = "Not Runtime";
	specification.WindowWidth = 1600;
	specification.WindowHeight = 900;
	specification.WindowDecorated = true;
	specification.Fullscreen = false;
	specification.StartMaximized = true;
	specification.VSync = true;
	specification.EnableImGui = false;
	specification.WorkingDirectory = "../NotEditor";

	return new RuntimeApplication(specification, projectPath);
}