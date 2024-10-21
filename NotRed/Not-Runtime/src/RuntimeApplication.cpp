#include <NotRed.h>
#include <NotRed/EntryPoint.h>

#include "RuntimeLayer.h"
#include "NotRed/Renderer/RendererAPI.h"

class RuntimeApplication : public NR::Application
{
public:
	RuntimeApplication(const NR::ApplicationSpecification& specification)
		: Application(specification)
	{
	}

	void Init() override
	{
		PushLayer(new NR::RuntimeLayer());
	}
};

NR::Application* NR::CreateApplication(int argc, char** argv)
{
	NR::ApplicationSpecification specification;
	specification.Name = "Not Runtime";
	specification.WindowWidth = 1600;
	specification.WindowHeight = 900;
	specification.Fullscreen = true;
	specification.VSync = true;
	specification.EnableImGui = false;
	specification.WorkingDirectory = "../NotEditor";

	return new RuntimeApplication(specification);
}