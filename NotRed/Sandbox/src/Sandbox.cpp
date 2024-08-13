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