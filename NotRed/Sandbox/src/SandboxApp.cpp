#include <NotRed.h>

class ExampleLayer : public NR::Layer
{
public:
	ExampleLayer()
		:Layer("Example")
	{
	}

	void Update() override
	{
		NR_INFO("Example Update");
	}

	void OnEvent(NR::Event& myEvent) override
	{
		NR_TRACE("{0}", myEvent);
	}
};

class Sandbox : public NR::Application
{
public:
	Sandbox()
	{
		PushOverlay(new NR::ImGuiLayer());
	}

	~Sandbox() override
	{

	}
};

NR::Application* NR::CreateApplication()
{
	return new Sandbox();
}