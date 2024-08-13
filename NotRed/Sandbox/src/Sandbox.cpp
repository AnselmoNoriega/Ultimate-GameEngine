#include "NotRed.h"

#include "NotRed/ImGui/ImGuiLayer.h"

class GameLayer : public NR::Layer
{
public:
	GameLayer()
	{
	}

	virtual ~GameLayer()
	{
	}

	virtual void Attach() override
	{
	}

	virtual void Detach() override
	{
	}

	virtual void Update() override
	{
		NR::Renderer::Clear(0.2f, 0.3f, 0.8f, 1);
	}

	virtual void OnEvent(NR::Event& event) override
	{
	}
};

class Sandbox : public NR::Application
{
public:
	Sandbox()
	{
		NR_TRACE("Hello!");
	}

	void Init() override
	{
		PushLayer(new GameLayer());
		PushOverlay(new NR::ImGuiLayer("ImGui"));
	}
};

NR::Application* NR::CreateApplication()
{
	return new Sandbox();
}