#include <NotRed.h>
#include <NotRed/EntryPoint.h>

#include "EditorLayer.h"

#include "NotRed/Renderer/RendererAPI.h"

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

NR::Application* NR::CreateApplication(int argc, char** argv)
{
	NR::RendererAPI::SetAPI(NR::RendererAPIType::OpenGL);
	return new NotEditorApplication({ "NotEditor", 1600, 900 });
}