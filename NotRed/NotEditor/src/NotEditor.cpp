#include <NotRed.h>
#include <NotRed/EntryPoint.h>

#include "EditorLayer.h"

class NotEditorApplication : public NR::Application
{
public:
	NotEditorApplication(const NR::ApplicationSpecification& specification)
		: Application(specification)
	{
	}

	void Init() override
	{
		PushLayer(new NR::EditorLayer());
	}
};

NR::Application* NR::CreateApplication(int argc, char** argv)
{
	NR::ApplicationSpecification specification;
	specification.Name = "NotEditor";
	specification.WindowWidth = 1600;
	specification.WindowHeight = 900;
	specification.VSync = true;
	return new NotEditorApplication(specification);
}