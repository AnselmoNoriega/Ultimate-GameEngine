#include <NotRed.h>
#include <NotRed/EntryPoint.h>

#include "EditorLayer.h"

class NotEditorApplication : public NR::Application
{
public:
	NotEditorApplication(const NR::ApplicationProps& props)
		: Application(props)
	{
	}

	void Init() override
	{
		PushLayer(new NR::EditorLayer());
	}
};

NR::Application* NR::CreateApplication()
{
	return new NotEditorApplication({ "NotEditor", 1600, 900 });
}